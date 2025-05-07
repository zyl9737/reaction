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

 // Forward declarations
 struct DataNode;
 struct ActionNode;
 struct LastValStrategy;
 struct AnyType;
 struct SimpleExpr;
 class FieldBase;
 class ObserverDataNode;
 class ObserverActionNode;

 template <typename TriggerPolicy, typename InvalidStrategy, typename Type, typename... Args>
 class ReactImpl;

 template <typename Op, typename L, typename R>
 class BinaryOpExpr;

 template <typename T>
 class React;

 template <typename T>
 struct ValueWrapper;

 // ==================== Helper Types ====================

 struct AnyType {
     template <typename T>
     operator T() { return T{}; }
 };

 // ==================== Type Traits ====================

 template <typename T>
 struct IsBinaryOpExpr : std::false_type {};

 template <typename Op, typename L, typename R>
 struct IsBinaryOpExpr<BinaryOpExpr<Op, L, R>> : std::true_type {};

 template <typename T>
 struct IsReact : std::false_type {
     using Type = T;
 };

 template <typename T>
 struct IsReact<React<T>> : std::true_type {
     using Type = T;
 };

// ==================== Basic type concepts ====================

 template <typename T>
 concept ConstType = std::is_const_v<std::decay_t<T>>;

 template <typename T>
 concept InvocableType = std::is_invocable_v<std::decay_t<T>>;

 template <typename T>
 concept NonInvocableType = !InvocableType<T>;

 template <typename T>
 concept VoidType = std::is_void_v<std::decay_t<T>>;

 template <typename T>
 concept ComparableType = requires(T &a, T &b) {
     { a == b } -> std::convertible_to<bool>;
 };

 template <typename... Args>
 concept HasArguments = sizeof...(Args) > 0;

 // ==================== Expression Traits ====================

 template <typename T>
 struct ExpressionTraits {
     using Type = T;
 };

 template <typename TriggerPolicy, typename InvalidStrategy, NonInvocableType T>
 struct ExpressionTraits<ReactImpl<TriggerPolicy, InvalidStrategy, T>> {
     using Type = T;
 };

 template <typename TriggerPolicy, typename InvalidStrategy, typename Fun, typename... Args>
 struct ExpressionTraits<ReactImpl<TriggerPolicy, InvalidStrategy, Fun, Args...>> {
     using Type = decltype(std::declval<Fun>()(std::declval<typename ExpressionTraits<Args>::Type>()...));
 };

 template <typename Fun, typename... Args>
 using ReturnType = typename ExpressionTraits<ReactImpl<void, void, std::decay_t<Fun>, std::decay_t<Args>...>>::Type;

 template <typename T>
 using ExprWrapper = std::conditional_t<
     IsReact<T>::value || IsBinaryOpExpr<T>::value,
     T,
     ValueWrapper<T>>;

 // ==================== Logic Concepts ====================

 template <typename T>
 concept IsObserverNode = requires {
     requires std::is_same_v<typename T::SourceType, DataNode> ||
              std::is_same_v<typename T::SourceType, ActionNode>;
 };

 template <typename T>
 concept IsReactSource = requires(T t) {
     requires requires { { t.getShared() } -> std::same_as<std::shared_ptr<ObserverDataNode>>; } ||
              requires { { t.getShared() } -> std::same_as<std::shared_ptr<ObserverActionNode>>; };
 };

 template <typename T>
 concept IsDataSource = requires {
     typename T::ValueType;
     requires IsReactSource<T> && !std::is_void_v<typename T::ValueType>;
 };

 template <typename T>
 concept IsSimpleExpr = std::is_same_v<T, SimpleExpr>;

 template <typename T>
 concept IsTriggerMode = requires(T t) {
     { t.checkTrigger() } -> std::same_as<bool>;
 };

 template <typename T>
 concept IsInvalidPolicy = requires(T t, AnyType ds) {
     { t.handleInvalid(ds) } -> std::same_as<void>;
 };

 template <typename T>
 concept HasField = std::is_base_of_v<FieldBase, std::decay_t<T>>;

 template <typename T>
 concept IsBinaryOpExpression = IsBinaryOpExpr<T>::value;

 template <typename L, typename R>
 concept HasCustomOp = IsReact<std::decay_t<L>>::value ||
                     IsReact<std::decay_t<R>>::value ||
                     IsBinaryOpExpr<std::decay_t<L>>::value ||
                     IsBinaryOpExpr<std::decay_t<R>>::value;

 } // namespace reaction

 #endif // REACTION_CONCEPT_H