/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_EXPRESSION_H
#define REACTION_EXPRESSION_H

#include "reaction/resource.h"
#include "reaction/triggerMode.h"

namespace reaction {

// Forward declaration of DataSource to be used in ExpressionTraits
template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
class DataSource;

// General template for ExpressionTraits
template <typename T>
struct ExpressionTraits {
    using Type = T;
};

// Specialization for DataSource without Functor (Base case)
template <typename TriggerPolicy, typename InvalidStrategy, UnInvocaCC T>
struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, T>> {
    using Type = T;
};

// Specialization for DataSource with Functor (Recursive case)
template <typename TriggerPolicy, typename InvalidStrategy, typename Fun, typename... Args>
struct ExpressionTraits<DataSource<TriggerPolicy, InvalidStrategy, Fun, Args...>> {
    using Type = decltype(std::declval<Fun>()(std::declval<typename ExpressionTraits<Args>::Type>()...));
};

// Helper alias to retrieve return type of functors
template <typename Fun, typename... Args>
using ReturnType = typename ExpressionTraits<DataSource<void, void, Fun, Args...>>::Type;

// General Expression class template (for functor-based expressions)
template <typename TriggerPolicy, typename Fun, typename... Args>
class Expression : public Resource<ComplexExpr, ReturnType<Fun, Args...>>, public TriggerPolicy {
public:
    using ValueType = ReturnType<Fun, Args...>;

protected:
    // Sets the source data for the expression and updates the functor
    template <typename F, typename... A>
    bool setSource(F &&f, A &&...args) {
        bool repeat = false;

        // Check if we should repeat the update
        if (!this->updateObservers(repeat, [this](bool changed) { this->valueChanged(changed); }, std::forward<A>(args)...)) {
            return false;
        }

        // Choose the right functor based on whether we repeat the update or not
        if (repeat) {
            setFunctor(createUpdateFunRef(std::forward<F>(f), std::forward<A>(args)...));
        } else {
            setFunctor(createGetFunRef(std::forward<F>(f), std::forward<A>(args)...));
        }

        // Handle trigger policy for threshold
        if constexpr (std::is_same_v<TriggerPolicy, ThresholdTrigger>) {
            TriggerPolicy::setRepeatDependent(repeat);
        }

        if constexpr (!std::is_void_v<ValueType>) {
            this->updateValue(evaluate());
        } else {
            evaluate();
        }
        return true;
    }

    void updateOneOb(DataNodePtr node) {
        bool repeat = false;
        this->updateOneObserver(repeat, [this](bool changed) { this->valueChanged(changed); }, node);
    }

    // Evaluates the functor and returns its result
    auto evaluate() const {
        if constexpr (VoidCC<ValueType>) {
            std::invoke(m_fun);
        } else {
            return std::invoke(m_fun);
        }
    }

    // Value change notification based on trigger policy
    void valueChanged(bool changed) {
        if constexpr (std::is_same_v<TriggerPolicy, ValueChangeTrigger>) {
            this->setChanged(changed);
        }

        if (TriggerPolicy::checkTrigger()) {
            if constexpr (!VoidCC<ValueType>) {
                auto oldVal = this->getValue();
                auto newVal = evaluate();
                this->updateValue(newVal);
                if constexpr (CompareCC<ValueType>) {
                    this->notifyObservers(oldVal != newVal);
                } else {
                    this->notifyObservers(true);
                }
            } else {
                evaluate();
            }
        }
    }

    // Set the functor for the expression
    void setFunctor(std::function<ValueType()> &&fun) {
        m_fun = std::move(fun);
    }

private:
    std::function<ValueType()> m_fun; // Functor for evaluation
};

// Specialized Expression class template (for simple value-based expressions)
template <typename TriggerPolicy, UnInvocaCC Type>
    requires(!IsBinaryOpExprCC<Type>)
class Expression<TriggerPolicy, Type> : public Resource<SimpleExpr, std::decay_t<Type>> {
public:
    using Resource<SimpleExpr, std::decay_t<Type>>::Resource;
    using ValueType = Type;

protected:
    // Direct value retrieval for simple expressions
    Type evaluate() const {
        return this->getValue();
    }
};

template <typename Op, typename L, typename R>
class BinaryOpExpr : public Expression<AlwaysTrigger, std::function<typename std::common_type_t<typename L::ValueType, typename R::ValueType>()>> {
    L left;
    R right;
    Op op;

public:
    using ValueType = typename std::common_type_t<typename L::ValueType, typename R::ValueType>;
    template <typename Left, typename Right>
    BinaryOpExpr(Left &&l, Right &&r, Op o = Op{}) :
        left(std::forward<Left>(l)), right(std::forward<Right>(r)), op(o) {
    }

    BinaryOpExpr(const BinaryOpExpr &expr) :
        left(expr.left), right(expr.right), op(expr.op) {
    }

    auto operator()() const {
        return calculate();
    }

protected:
    void setOpExpr() {
        this->setFunctor([this]() {
            return this->calculate();
        });
        this->updateValue(this->evaluate());
    }

    auto calculate() const {
        return op(left(), right());
    }
};

struct AddOp {
    auto operator()(auto &&l, auto &&r) const {
        return l + r;
    }
};

struct MulOp {
    auto operator()(auto &&l, auto &&r) const {
        return l * r;
    }
};

struct SubOp {
    auto operator()(auto &&l, auto &&r) const {
        return l - r;
    }
};

struct DivOp {
    auto operator()(auto &&l, auto &&r) const {
        return l / r;
    }
};

template <typename T>
struct ValueWrapper {
    using ValueType = T;
    T value;

    template <typename Type>
    ValueWrapper(Type &&t) :
        value(std::forward<Type>(t)) {
    }
    const T &operator()() const {
        return value;
    }
};

template <typename T>
using ExprWrapper = std::conditional_t<
    is_data_weak_ref<T>::value || is_binary_op_expr<T>::value,
    T,
    ValueWrapper<T>>;

template <typename Op, typename L, typename R>
auto make_binary_expr(L &&l, R &&r) {
    return BinaryOpExpr<Op, ExprWrapper<std::decay_t<L>>, ExprWrapper<std::decay_t<R>>>(
        std::forward<L>(l),
        std::forward<R>(r));
}

template <typename L, typename R>
    requires CustomOpCC<L, R>
auto operator+(L &&l, R &&r) {
    return make_binary_expr<AddOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires CustomOpCC<L, R>
auto operator*(L &&l, R &&r) {
    return make_binary_expr<MulOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires CustomOpCC<L, R>
auto operator-(L &&l, R &&r) {
    return make_binary_expr<SubOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename L, typename R>
    requires CustomOpCC<L, R>
auto operator/(L &&l, R &&r) {
    return make_binary_expr<DivOp>(std::forward<L>(l), std::forward<R>(r));
}

template <typename TriggerPolicy, typename Op, typename L, typename R>
class Expression<TriggerPolicy, BinaryOpExpr<Op, L, R>> : public BinaryOpExpr<Op, L, R> {
public:
    using ValueType = BinaryOpExpr<Op, L, R>::ValueType;
    Expression(const BinaryOpExpr<Op, L, R> &expr) :
        BinaryOpExpr<Op, L, R>(expr) {
    }
};

} // namespace reaction

#endif // REACTION_EXPRESSION_H
