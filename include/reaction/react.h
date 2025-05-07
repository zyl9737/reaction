/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_DATASOURCE_H
#define REACTION_DATASOURCE_H

#include "reaction/expression.h"
#include "reaction/invalidStrategy.h"

namespace reaction {

inline thread_local bool g_reg_flg = false;
inline thread_local std::function<void(DataNodePtr)> g_reg_fun;

struct RegGuard {
    RegGuard() {
        g_reg_flg = true;
    }
    ~RegGuard() {
        g_reg_flg = false;
        g_reg_fun = nullptr;
    }
};

template <typename ReactType>
class React {
public:
    using InvStrategy = typename ReactType::InvStrategy;
    using ValueType = typename ReactType::ValueType;
    using ExprType = typename ReactType::ExprType;

    // Constructor: increases weak reference count
    explicit React(std::shared_ptr<ReactType> ptr = nullptr) :
        m_weakPtr(ptr) {
            if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    // Destructor that releases the weak reference
    ~React() {
        if (auto p = m_weakPtr.lock())
            p->releaseWeakRef();
    }

    // Copy constructor, adding a weak reference
    React(const React &other) :
        m_weakPtr(other.m_weakPtr) {
        if (auto p = m_weakPtr.lock())
            p->addWeakRef();
    }

    // Move constructor
    React(React &&other) noexcept :
        m_weakPtr(std::move(other.m_weakPtr)) {
        other.m_weakPtr.reset();
    }

    // Copy assignment operator, adding a weak reference
    React &operator=(const React &other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = other.m_weakPtr;
            if (auto p = m_weakPtr.lock())
                p->addWeakRef();
        }
        return *this;
    }

    // Move assignment operator
    React &operator=(React &&other) noexcept {
        if (this != &other) {
            if (auto p = m_weakPtr.lock())
                p->releaseWeakRef();
            m_weakPtr = std::move(other.m_weakPtr);
            other.m_weakPtr.reset();
        }
        return *this;
    }

    // Arrow operator: accesses raw pointer's members
    auto operator->() const {
        return getPtr()->getRaw();
    }

    auto operator()() const {
        if (g_reg_flg) {
            std::invoke(g_reg_fun, getPtr()); // Call registration function
        }
        return get();
    }

    // Dereference operator: returns the underlying object
    ReactType &operator*() const {
        return *getPtr(); // Dereference the raw pointer
    }

    // Explicit conversion to bool: checks if the raw pointer is valid
    explicit operator bool() const {
        return !m_weakPtr.expired(); // Returns true if the pointer is not null
    }

    // Getter functions for accessing data from the ReactImpl
    auto get() const
        requires IsDataSource<ReactType>
    {
        return getPtr()->getValue(); // Get the value of the data source
    }

    auto getRaw() const
        requires IsDataSource<ReactType>
    {
        return getPtr()->getRaw(); // Get the raw value of the data source
    }

    decltype(auto) getRef() const
        requires IsDataSource<ReactType>
    {
        return getPtr()->getRef(); // Get the reference to the data source
    }

    auto getShared() {
        return getPtr()->getShared(); // Get a shared pointer to the data source
    }

    template <typename T>
    React &value(T &&t) {
        getPtr()->value(std::forward<T>(t));
        return *this;
    }

    template <typename F, typename... A>
    ReactionError set(F &&f, A &&...args) {
        return getPtr()->set(std::forward<F>(f), std::forward<A>(args)...); // Set value in the data source
    }

    // Set a threshold for the source
    template <typename F, typename... A>
    React &setThreshold(F &&f, A &&...args) {
        getPtr()->setThreshold(std::forward<F>(f), std::forward<A>(args)...); // Set threshold for the data source
        return *this;
    }

    // Close the data source
    React &close() {
        getPtr()->close(); // Close the data source
        return *this;
    }

    // Set and get the name of the data source
    React &setName(const std::string &name) {
        getPtr()->setName(name); // Set the name of the data source
        return *this;
    }

    std::string getName() const {
        return getPtr()->getName(); // Get the name of the data source
    }

protected:
    // Helper function to safely access the raw pointer, throws if null
    std::shared_ptr<ReactType> getPtr() const {
        if (m_weakPtr.expired()) {
            throw std::runtime_error("Null weak pointer access"); // Throws if the pointer is null
        }
        return m_weakPtr.lock(); // Returns the raw pointer
    }

    std::weak_ptr<ReactType> m_weakPtr; // Initialize the pointer to null
};

// ReactImpl class template that handles the value, observers, and invalidation strategies
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
class ReactImpl : public Expression<TriggerPolicy, Type, Args...>,
                  public InvalidStrategy {
public:
    using Expr = Expression<TriggerPolicy, Type, Args...>;
    using ValueType = typename Expr::ValueType;
    using ExprType = typename Expr::ExprType;
    using InvStrategy = InvalidStrategy;
    using Expr::Expr; // Inherit constructors from Expr

    // Assignment operator to update the value and notify observers
    template <typename T>
    ReactImpl &operator=(T &&t)
        requires(IsSimpleExpr<ExprType>)
    {
        value(std::forward<T>(t));
        return *this;
    }

    // Addition assignment operator
    template <typename T>
    ReactImpl &operator+=(const T &rhs)
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() + rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Subtraction assignment operator
    template <typename T>
    ReactImpl &operator-=(const T &rhs)
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() - rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Multiplication assignment operator
    template <typename T>
    ReactImpl &operator*=(const T &rhs)
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() * rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Division assignment operator
    template <typename T>
    ReactImpl &operator/=(const T &rhs)
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() / rhs);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix increment operator
    ReactImpl &operator++()
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() + 1);
        this->notifyObservers(true);
        return *this;
    }

    // Prefix decrement operator
    ReactImpl &operator--()
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(this->getValue() - 1);
        this->notifyObservers(true);
        return *this;
    }

    // Set new value and notify observers
    template <typename F, HasArguments... A>
    ReactionError set(F &&f, A &&...args) {
        return this->setSource(std::forward<F>(f), std::forward<A>(args)...);
    }

    template <InvocableType F>
    ReactionError set(F &&f) {
        RegGuard guard;
        g_reg_fun = [this](DataNodePtr node) {
            this->updateOneOb(node);
        };
        return this->setSource(std::forward<F>(f));
    }

    ReactionError set() {
        RegGuard guard;
        g_reg_fun = [this](DataNodePtr node) {
            this->updateOneOb(node);
        };
        return this->setOpExpr();
    }

    // Close the ReactImpl and remove it from the observer graph
    void close() {
        ObserverGraph::getInstance().closeNode(this->getShared());
    }

    template <typename T>
    void value(T &&t)
        requires(IsSimpleExpr<ExprType> && !ConstType<ValueType>)
    {
        this->updateValue(std::forward<T>(t));
        if constexpr (ComparableType<ValueType>) {
            this->notifyObservers(this->getValue() != t);
        } else {
            this->notifyObservers(true);
        }
    }

    // Getter functions
    auto get() const {
        return this->getValue();
    }
    auto getRaw() const {
        return this->getRawPtr();
    }
    decltype(auto) getRef() const {
        return this->getReference();
    }

private:
    void addWeakRef() {
        m_weakRefCount++;
    }

    void releaseWeakRef() {
        if (--m_weakRefCount == 0) {
            this->handleInvalid(*this);
        }
    }

    template <typename T>
    friend class React;

    // Atomic counter for weak references
    std::atomic<int> m_weakRefCount{0};
};

} // namespace reaction

#endif // REACTION_DATASOURCE_H
