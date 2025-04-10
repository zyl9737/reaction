/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_CONCEPT_H
#define REACTION_CONCEPT_H
#include <type_traits>
#include <memory>
#include <concepts>

namespace reaction {

struct AnyType {
    // Conversion operator to allow AnyType to be used as any type T
    template <typename T>
    operator T() {
        return T{};
    }
};

template <typename T>
concept TriggerCC = requires(T t) {
    { t.checkTrigger() } -> std::same_as<bool>;
};

struct DataNode;
struct ActionNode;
struct FieldNode;
template <typename T>
concept NodeCC = requires(T t) {
    requires std::is_same_v<typename T::SourceType, DataNode>
                 || std::is_same_v<typename T::SourceType, ActionNode>
                 || std::is_same_v<typename T::SourceType, FieldNode>;
};

class ObserverDataNode;
class ObserverActionNode;
class ObserverFieldNode;
template <typename T>
concept SourceCC = requires(T t) {
    requires requires {
        { t.getShared() } -> std::same_as<std::shared_ptr<ObserverDataNode>>;
    } || requires {
        { t.getShared() } -> std::same_as<std::shared_ptr<ObserverActionNode>>;
    } || requires {
        { t.getShared() } -> std::same_as<std::shared_ptr<ObserverFieldNode>>;
    };
};

template <typename T>
concept ActionSourceCC = requires {
    typename T::ExprType::ValueType;
    requires SourceCC<T> && std::is_same_v<typename T::ExprType::ValueType, void>;
};

template <typename T>
concept DataSourceCC = requires {
    typename T::ExprType::ValueType;
    requires SourceCC<T> && !std::is_same_v<typename T::ExprType::ValueType, void>;
};

template <typename T>
concept InvalidCC = requires(T t) {
    requires requires(AnyType ds) {
        { t.handleInvalid(ds) } -> std::same_as<void>;
    };
};

template <typename T>
concept ConstCC = std::is_const_v<T>;

template <typename T>
concept InvocaCC = std::is_invocable_v<std::decay_t<T>>;

template <typename T>
concept VoidCC = std::is_void_v<T>;

template <typename T>
concept UninvocaCC = !InvocaCC<T>;

template <typename... Args>
concept ArgSizeOverZeroCC = sizeof...(Args) > 0;

struct FieldStructBase;
template <typename T>
concept HasFieldCC = std::is_base_of_v<FieldStructBase, T>;

template <typename T>
concept CompareCC = requires(T &a, T &b) {
    { a == b } -> std::convertible_to<bool>;
};

template <typename Op, typename L, typename R>
class BinaryOpExpr;

template <typename T>
struct is_binary_op_expr : std::false_type {};

template <typename Op, typename L, typename R>
struct is_binary_op_expr<BinaryOpExpr<Op, L, R>> : std::true_type {};

template <typename T>
concept IsBinaryOpExprCC = is_binary_op_expr<T>::value;

template <typename T>
class DataWeakRef;

template <typename T>
struct is_data_weak_ref : std::false_type {
    using Type = T;
};

template <typename T>
struct is_data_weak_ref<DataWeakRef<T>> : std::true_type {
    using Type = T;
};

template <typename L, typename R>
concept CustomOpCC =
    is_data_weak_ref<std::decay_t<L>>::value ||
    is_data_weak_ref<std::decay_t<R>>::value ||
    is_binary_op_expr<std::decay_t<L>>::value ||
    is_binary_op_expr<std::decay_t<R>>::value;

struct LastValStrategy;
template <typename T>
concept VarInvalidCC = InvalidCC<T> && !std::is_same_v<T, LastValStrategy>;

} // namespace reaction

#endif // REACTION_CONCEPT_H