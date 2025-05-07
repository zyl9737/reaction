/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_REACTION_H
#define REACTION_REACTION_H

#include "reaction/react.h"

namespace reaction {
// Field alias for React to work with ReactImpl
template <typename SourceType>
using Field = React<ReactImpl<AlwaysTrigger, DirectCloseStrategy, std::decay_t<SourceType>>>;

// Base struct for fields (empty base class)
class FieldBase {
public:
    template <typename T>
    auto field(T &&t) {
        auto ptr = std::make_shared<ReactImpl<AlwaysTrigger, DirectCloseStrategy, std::decay_t<T>>>(std::forward<T>(t));
        FieldGraph::getInstance().addObj(m_id, ptr->getShared());
        return React{ptr};
    }
    uint64_t getId() {
        return m_id;
    }

private:
    UniqueID m_id;
};

// Function to create a constant ReactImpl
template <IsTriggerMode TriggerPolicy = AlwaysTrigger, IsInvalidPolicy InvalidStrategy = DirectCloseStrategy, typename SourceType>
auto constVar(SourceType &&value) {
    auto ptr = std::make_shared<ReactImpl<TriggerPolicy, InvalidStrategy, const std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    ObserverGraph::getInstance().addNode(ptr);
    return React{ptr};
}

// Function to create a var ReactImpl
template <IsTriggerMode TriggerPolicy = AlwaysTrigger, IsInvalidPolicy InvalidStrategy = DirectCloseStrategy, typename SourceType>
auto var(SourceType &&value) {
    auto ptr = std::make_shared<ReactImpl<TriggerPolicy, InvalidStrategy, std::decay_t<SourceType>>>(std::forward<SourceType>(value));
    if constexpr (HasField<std::decay_t<SourceType>>) {
        ptr->setField();
    }
    ObserverGraph::getInstance().addNode(ptr);
    return React{ptr};
}

template <IsTriggerMode TriggerPolicy = AlwaysTrigger, IsInvalidPolicy InvalidStrategy = DirectCloseStrategy, IsBinaryOpExpression OpExpr>
auto expr(OpExpr &&opExpr) {
    auto ptr = std::make_shared<ReactImpl<TriggerPolicy, InvalidStrategy, std::decay_t<OpExpr>>>(std::forward<OpExpr>(opExpr));
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set();
    return React{ptr};
}

// Function to create a variable ReactImpl
template <IsTriggerMode TriggerPolicy = AlwaysTrigger, IsInvalidPolicy InvalidStrategy = DirectCloseStrategy, typename Fun, typename... Args>
auto calc(Fun &&fun, Args &&...args) {
    auto ptr = std::make_shared<ReactImpl<TriggerPolicy, InvalidStrategy, std::decay_t<Fun>, typename IsReact<std::decay_t<Args>>::Type...>>();
    ObserverGraph::getInstance().addNode(ptr);
    ptr->set(std::forward<Fun>(fun), std::forward<Args>(args)...);
    return React{ptr};
}

// Function to create an action ReactImpl
template <IsTriggerMode TriggerPolicy = AlwaysTrigger, IsInvalidPolicy InvalidStrategy = DirectCloseStrategy, typename Fun, typename... Args>
auto action(Fun &&fun, Args &&...args) {
    return calc<TriggerPolicy, InvalidStrategy>(std::forward<Fun>(fun), std::forward<Args>(args)...);
}

} // namespace reaction

#endif // REACTION_REACTION_H