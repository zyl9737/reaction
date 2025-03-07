#ifndef REACTION_INVALIDSTRATEGY_H
#define REACTION_INVALIDSTRATEGY_H

#include "reaction/observerNode.h"

namespace reaction
{
    template <typename Type, typename... Args>
    class DataSource;

    enum class InvalidStrategyType
    {
        DirectFailure,
        UseLastValidValue,
        ContinueWithExpression
    };

    struct DirectFailureStrategy
    {
        template <typename... Args>
        static void handleInvalid(std::shared_ptr<ObserverNode> node, Args &&...args)
        {
            ObserverGraph::getInstance().closeNode(node);
        }
    };

    class UseLastValidValueStrategy
    {
    public:
        template <typename... Args>
        static void handleInvalid(std::shared_ptr<ObserverNode> node, Args &&...args)
        {
            auto ptr = std::make_shared<DataSource<std::decay_t<Args>...>>(std::forward<Args>(args)...);
            ObserverGraph::getInstance().moveNode(ptr, node);
        }
    };

    class ContinueWithExpressionStrategy
    {
    public:
        template <typename... Args>
        static void handleInvalid(std::shared_ptr<ObserverNode> node, Args &&...args) {}
    };
} // namespace reaction

#endif // REACTION_INVALIDSTRATEGY_H
