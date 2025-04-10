/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_INVALIDSTRATEGY_H
#define REACTION_INVALIDSTRATEGY_H

#include "reaction/observerNode.h"

namespace reaction {

// Strategy that handles invalid states by closing the node
struct DirectCloseStrategy {
    // Handle invalid node by closing it in the ObserverGraph
    template <typename Source>
    void handleInvalid(Source &&source) {
        if constexpr (SourceCC<std::decay_t<Source>>) {
            ObserverGraph::getInstance().closeNode(source.getShared());
        }
    }
};

// Strategy that keeps calculating without handling invalid state
struct KeepCalcStrategy {
    // No specific handling for invalid state
    template <typename Source>
    void handleInvalid([[maybe_unused]] Source &&source) {
    }
};

// Strategy that keeps the last valid value when the node is invalid
struct LastValStrategy {
    // Handle invalid node by setting the last valid value
    template <typename DS>
    void handleInvalid(DS &&ds) {
        if constexpr (DataSourceCC<std::decay_t<DS>>) {
            auto val = ds.get();           // Get the current valid value
            ds.set([=]() { return val; }); // Set the value to the last valid one
        }
    }
};

// Strategy for fields that closes the node in the FieldGraph when invalid
struct FieldStrategy {
    // Handle invalid node by closing it in the FieldGraph
    template <typename DS>
    void handleInvalid(DS &&ds) {
        if constexpr (DataSourceCC<std::decay_t<DS>>) {
            FieldGraph::getInstance().closeNode(ds.getShared());
        }
    }
};

} // namespace reaction

#endif // REACTION_INVALIDSTRATEGY_H
