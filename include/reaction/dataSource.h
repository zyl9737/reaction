/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/invalidStrategy.h"
#include "reaction/expression.h"
#include <atomic>

namespace reaction {

inline thread_local bool reg_flg = false;
inline thread_local std::function<void(DataNodePtr)> reg_fun;

struct RegGuard {
    RegGuard() {
        reg_flg = false;
    }
    ~RegGuard() {
        reg_flg = false;
        reg_fun = nullptr;
    }
};

// Trait to determine the expression type for DataSource
template <typename... Args>
struct SourceTraits {
    using ExprType = Expression<Args...>;
};

// Specialization of SourceTraits for DataSource with a single type
template <typename TriggerPolicy, typename InvalidStrategy, typename Type>
struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type>> {
    using ExprType = Expression<TriggerPolicy, Type>;
};

// Specialization of SourceTraits for DataSource with multiple types
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
struct SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>> {
    using ExprType = Expression<TriggerPolicy, Type, Args...>;
};

template <typename DataType>
class DataWeakRef {
public:
    using InvStrategy = typename DataType::InvStrategy;
    using ValueType = typename DataType::ExprType::ValueType;

    // Constructor: increases weak reference count
    explicit DataWeakRef(DataType *ptr) :
        m_rawPtr(ptr) {
        if (m_rawPtr) {
            m_rawPtr->addWeakRef(this, m_cb); // Increment weak reference count in DataType
        }
    }

    // Destructor: decreases weak reference count
    ~DataWeakRef() {
        if (m_rawPtr) {
            m_rawPtr->releaseWeakRef(this); // Decrement weak reference count in DataType
        }
    }

    // Copy constructor: increases weak reference count
    DataWeakRef(const DataWeakRef &other) :
        m_rawPtr(other.m_rawPtr) {
        if (m_rawPtr) {
            m_rawPtr->addWeakRef(this, m_cb); // Increment weak reference count in DataType
        }
    }

    // Move constructor: transfers ownership, nullifies the source pointer
    DataWeakRef(DataWeakRef &&other) noexcept :
        m_rawPtr(other.m_rawPtr) {
        other.m_rawPtr = nullptr; // Nullify the moved-from object
    }

    // Copy assignment: increases weak reference count after releasing current reference
    DataWeakRef &operator=(const DataWeakRef &other) noexcept {
        if (this != &other) {
            if (m_rawPtr) {
                m_rawPtr->releaseWeakRef(this); // Decrement weak reference count
            }
            m_rawPtr = other.m_rawPtr;
            if (m_rawPtr) {
                m_rawPtr->addWeakRef(this, m_cb); // Increment weak reference count
            }
        }
        return *this;
    }

    // Move assignment: transfers ownership, nullifies the source pointer
    DataWeakRef &operator=(DataWeakRef &&other) noexcept {
        if (this != &other) {
            if (m_rawPtr) {
                m_rawPtr->releaseWeakRef(this); // Decrement weak reference count
            }
            m_rawPtr = other.m_rawPtr;
            other.m_rawPtr = nullptr; // Nullify the moved-from object
        }
        return *this;
    }

    // Arrow operator: accesses raw pointer's members
    auto operator->() const {
        return getPtr()->getRaw();
    }

    auto operator()() const {
        if constexpr (!std::is_same_v<typename DataType::InvStrategy, FieldStrategy>) {
            if (reg_flg && m_rawPtr) {
                std::invoke(reg_fun, m_rawPtr->getShared()); // Call registration function
            }
        }
        return get();
    }

    // Dereference operator: returns the underlying object
    DataType &operator*() const {
        return *getPtr(); // Dereference the raw pointer
    }

    // Explicit conversion to bool: checks if the raw pointer is valid
    explicit operator bool() const {
        return m_rawPtr != nullptr; // Returns true if the pointer is not null
    }

    // Getter functions for accessing data from the DataSource
    auto get() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getValue(); // Get the value of the data source
    }

    auto getUpdate() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->evaluate(); // Get the updated value of the data source
    }

    auto getRaw() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getRaw(); // Get the raw value of the data source
    }

    auto &getRef() const
        requires DataSourceCC<DataType>
    {
        return getPtr()->getRef(); // Get the reference to the data source
    }

    auto getShared() {
        return getPtr()->getShared(); // Get a shared pointer to the data source
    }

    template <typename T>
    void value(T &&t) {
        getPtr()->value(std::forward<T>(t));
    }

    template <typename F, typename... A>
    bool set(F &&f, A &&...args) {
        return getPtr()->set(std::forward<F>(f), std::forward<A>(args)...); // Set value in the data source
    }

    // Set a threshold for the source
    template <typename F, typename... A>
    void setThreshold(F &&f, A &&...args) {
        getPtr()->setThreshold(std::forward<F>(f), std::forward<A>(args)...); // Set threshold for the data source
    }

    // Close the data source
    void close() {
        getPtr()->close(); // Close the data source
    }

    // Set and get the name of the data source
    void setName(const std::string &name) {
        getPtr()->setName(name); // Set the name of the data source
    }

    std::string getName() const {
        return getPtr()->getName(); // Get the name of the data source
    }

private:
    // Helper function to safely access the raw pointer, throws if null
    DataType *getPtr() const {
        if (!m_rawPtr) {
            throw std::runtime_error("Null weak pointer access"); // Throws if the pointer is null
        }
        return m_rawPtr; // Returns the raw pointer
    }

    // Raw pointer to the DataSource object
    DataType *m_rawPtr = nullptr; // Initialize the pointer to null
    std::function<void()> m_cb = [this]() {
        m_rawPtr = nullptr;
    };
};

// DataSource class template that handles the value, observers, and invalidation strategies
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
class DataSource : public SourceTraits<DataSource<TriggerPolicy, InvalidStrategy, Type, Args...>>::ExprType,
                   private InvalidStrategy {
public:
    // Using the ExprType from SourceTraits
    using ExprType = typename SourceTraits<DataSource>::ExprType;
    using InvStrategy = InvalidStrategy;
    using ExprType::ExprType; // Inherit constructors from ExprType

    ~DataSource() {
        notifyDestruction();
    }

    // Set new value and notify observers
    template <typename F, ArgSizeOverZeroCC... A>
    bool set(F &&f, A &&...args) {
        return this->setSource(std::forward<F>(f), std::forward<A>(args)...);
    }

    template <InvocaCC F>
    bool set(F &&f) {
        RegGuard guard;
        reg_flg = true;
        reg_fun = [this](DataNodePtr node) {
            this->updateOneOb(node);
        };
        return this->setSource(std::forward<F>(f));
    }

    bool set() {
        RegGuard guard;
        reg_flg = true;
        reg_fun = [this](DataNodePtr node) {
            this->updateOneOb(node);
        };
        this->setOpExpr();
        return true;
    }

    // Close the DataSource and remove it from the observer graph
    void close() {
        ObserverGraph::getInstance().closeNode(this->getShared());
    }

    template <typename Trigger, typename Invalid, typename T, typename... A>
    auto operator+(const DataSource<Trigger, Invalid, T, A...> &data)
        requires(!ConstCC<Type>)
    {
        return get() + data.get();
    }

    // Assignment operator to update the value and notify observers
    template <typename T>
    DataSource &operator=(T &&t)
        requires(!ConstCC<Type>)
    {
        value(std::forward<T>(t));
        return *this;
    }

    // Addition assignment operator
    DataSource &operator+=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() + rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Subtraction assignment operator
    DataSource &operator-=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() - rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Multiplication assignment operator
    DataSource &operator*=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() * rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Division assignment operator
    DataSource &operator/=(const Type &rhs)
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() / rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix increment operator
    DataSource &operator++()
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() + 1);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix decrement operator
    DataSource &operator--()
        requires(!ConstCC<Type>)
    {
        this->updateValue(this->getValue() - 1);
        this->notifyObservers(true);
        return *this;
    }

    template <typename T>
    void value(T &&t)
        requires(!ConstCC<Type>)
    {
        this->updateValue(std::forward<T>(t));
        this->notifyObservers(this->getValue() != t);
    }

    // Getter functions
    auto get() const {
        return this->getValue();
    }
    auto getUpdate() const {
        return this->evaluate();
    }
    auto getRaw() const {
        return this->getRawPtr();
    }
    auto &getRef() const {
        return this->getReference();
    }

private:
    // Friend class to allow access to private methods for weak reference management
    template <typename T>
    friend class DataWeakRef;

    // Methods for managing weak references
    void addWeakRef(DataWeakRef<DataSource> *ptr, std::function<void()> &f) {
        m_weakRefCount++;
        m_destructionCallbacks.insert({ptr, f});
    }

    void releaseWeakRef(DataWeakRef<DataSource> *ptr) {
        --m_weakRefCount;
        m_destructionCallbacks.erase(ptr);
        if (m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

    void notifyDestruction() {
        for (auto &[ptr, cb] : m_destructionCallbacks) {
            cb();
        }
    }
    // Atomic counter for weak references
    std::atomic<int> m_weakRefCount{0};
    std::unordered_map<DataWeakRef<DataSource> *, std::function<void()>> m_destructionCallbacks;
};

// Field alias for DataWeakRef to work with DataSource
template <CompareCC SourceType>
using Field = DataWeakRef<DataSource<AlwaysTrigger, FieldStrategy, FieldIdentity<std::decay_t<SourceType>>>>;

// Function to create a Field DataSource
template <CompareCC SourceType>
auto field(FieldStructBase *meta, SourceType &&value) {
    auto ptr = std::make_shared<DataSource<AlwaysTrigger, FieldStrategy, FieldIdentity<std::decay_t<SourceType>>>>
               (FieldIdentity<std::decay_t<SourceType>>{meta, std::forward<SourceType>(value)});
    FieldGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr.get()};
}

// Function to create a constant DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, VarInvalidCC InvalidStrategy = DirectCloseStrategy, typename SourceType>
auto constVar(SourceType &&value) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, const std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    ObserverGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr.get()};
}

// Function to create a var DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, VarInvalidCC InvalidStrategy = DirectCloseStrategy, CompareCC SourceType>
auto var(SourceType &&value) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    if constexpr (HasFieldCC<std::decay_t<SourceType>>) {
        ptr->setField();
    }
    ObserverGraph::getInstance().addNode(ptr);
    return DataWeakRef{ptr.get()};
}

template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectCloseStrategy, IsBinaryOpExprCC OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, std::decay_t<OpExpr>>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return DataWeakRef{ptr.get()};
}

// Function to create a variable DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectCloseStrategy, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<DataSource<TriggerPolicy, InvalidStrategy, Fun, typename is_data_weak_ref<std::decay_t<Args>>::Type...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return DataWeakRef{ptr.get()};
}

// Function to create an action DataSource
template <TriggerCC TriggerPolicy = AlwaysTrigger, InvalidCC InvalidStrategy = DirectCloseStrategy, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TriggerPolicy, InvalidStrategy>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

} // namespace reaction

#endif // REACTION_DATASOURCE_H
