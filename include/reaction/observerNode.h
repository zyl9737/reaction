/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_OBSERVERNODE_H
#define REACTION_OBSERVERNODE_H

#include "reaction/observerHelper.h"
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace reaction {

// FieldGraph handles field-specific operations
class FieldGraph {
public:
    static FieldGraph &getInstance() {
        static FieldGraph instance;
        return instance;
    }

    // Add a field node to the graph
    void addNode(FieldNodePtr node) {
        m_fieldList.insert(node);
    }

    // Close a field node in the graph
    void closeNode(FieldNodePtr node) {
        m_fieldList.erase(node);
    }

    // Add a field to the graph
    void addObj(FieldStructBase *obj, ObserverFieldNode *node) {
        m_fieldMap[obj].insert(node);
    }

    // Delete a field from the graph
    void deleteObj(FieldStructBase *obj) {
        m_fieldMap.erase(obj);
    }

    // Set the field associated with a objdata pointer
    void setField(FieldStructBase *obj, DataNodePtr objPtr) {
        for (auto node : m_fieldMap[obj]) {
            m_fieldObservers[objPtr].insert(node);
        }
    }

    void deleteObservers(DataNodePtr node) {
        m_fieldObservers.erase(node);
    }

    // Get the objdata for a field node
    std::unordered_set<ObserverFieldNode *> getObservers(DataNodePtr node) {
        auto it = m_fieldObservers.find(node);
        if (it != m_fieldObservers.end()) {
            return it->second;
        } else {
            return {};
        }
    }

private:
    FieldGraph() {
    }
    std::unordered_map<FieldStructBase *, std::unordered_set<ObserverFieldNode *>> m_fieldMap;
    std::unordered_map<DataNodePtr, std::unordered_set<ObserverFieldNode *>> m_fieldObservers;
    std::unordered_set<FieldNodePtr> m_fieldList;
};

// ObserverGraph handles dependencies between nodes and manages observers
class ObserverGraph {
public:
    // Singleton instance of ObserverGraph
    static ObserverGraph &getInstance() {
        static ObserverGraph instance;
        return instance;
    }

    // Add a new node to the graph
    template <NodeCC NodeType>
    void addNode(std::shared_ptr<NodeType> node) {
        m_dependents[node];
        m_observers[node];
    }

    // Add an observer for a specific node
    template <NodeCC NodeType, NodeCC TargetType>
    bool addObserver(bool &repeat, std::function<void(bool)> &f, std::shared_ptr<NodeType> source, std::shared_ptr<TargetType> target) {
        if constexpr (std::is_same_v<NodeType, TargetType>) {
            if (source == target) {
                Log::error("Cannot observe self, node = {}.", source->getName());
                return false;
            }
        }
        ObserverCallback cb{f};

        if constexpr (std::is_same_v<typename NodeType::SourceType, DataNode>) {
            if (hasCycle(source, target)) {
                Log::error("Cycle dependency detected, node = {}. Cycle dependent = {}", source->getName(), target->getName());
                return false;
            }

            if (auto obs = FieldGraph::getInstance().getObservers(target); !obs.empty()) {
                for (auto &ob : obs) {
                    ob->addCb(cb);
                }
            }
        }

        if (!repeat && hasRepeatDependencies(source, target)) {
            Log::info("Repeat dependency detected, node = {}. Repeat dependent = {}", source->getName(), target->getName());
            repeat = true;
        }

        m_dependents[source].insert(target);
        m_observers[target].insert({source, cb});
        target->addCb(cb);
        return true;
    }

    // Reset the observers for a node
    template <NodeCC NodeType>
    void resetNode(std::shared_ptr<NodeType> node) {
        cleanupDependencies(node);
        m_dependents[node].clear();
    }

    // Close a node and its dependencies
    template <NodeCC NodeType>
    void closeNode(std::shared_ptr<NodeType> node) {
        if (!node) return;
        std::unordered_set<std::shared_ptr<void>> closedNodes;
        cascadeCloseDependents(node, closedNodes);
    }

private:
    ObserverGraph() {
    }

    template <NodeCC NodeType>
    void cleanupDependencies(std::shared_ptr<NodeType> node) {
        for (auto dep : m_dependents[node]) {
            auto it = m_observers[dep].find(node);
            if (it != m_observers[dep].end()) {
                std::visit([&](auto &&ptr) {
                    ptr->deleteCb(it->second);
                    if constexpr (std::is_same_v<std::decay_t<decltype(ptr)>, DataNodePtr>) {
                        if (auto obs = FieldGraph::getInstance().getObservers(ptr); !obs.empty()) {
                            for (auto &ob : obs) {
                                ob->deleteCb(it->second);
                            }
                        }
                    }
                }, dep);
            }
            m_observers[dep].erase(node);
        }
    }

    // Cascade close all dependent nodes
    template <NodeCC NodeType>
    void cascadeCloseDependents(std::shared_ptr<NodeType> node, std::unordered_set<std::shared_ptr<void>> &closedNodes) {
        if (!node || closedNodes.count(node)) return;
        closedNodes.insert(node);

        auto observers = m_observers[node];
        for (auto &[ob, _] : observers) {
            std::visit([&](auto &&ptr) {
                cascadeCloseDependents(ptr, closedNodes);
            }, ob);
        }

        closeNodeInternal(node);
    }

    // Internal close node implementation (without cascading)
    template <NodeCC NodeType>
    void closeNodeInternal(std::shared_ptr<NodeType> node) {
        if (!node) return;

        cleanupDependencies(node);
        m_dependents.erase(node);
        for (auto &[ob, cb] : m_observers[node]) {
            m_dependents[ob].erase(node);
        }
        m_observers.erase(node);

        node.reset();
    }

    // Check for cycle dependency between nodes
    bool hasCycle(DataNodePtr source, DataNodePtr target) {
        m_dependents[source].insert(target);
        m_observers[target].insert({source, ObserverCallback{}});

        std::unordered_set<DataNodePtr> visited;
        std::unordered_set<DataNodePtr> recursionStack;
        bool hasCycle = dfs(source, visited, recursionStack);

        m_dependents[source].erase(target);
        m_observers[target].erase(source);

        return hasCycle;
    }

    // Depth-first search to detect cycles
    bool dfs(DataNodePtr node, std::unordered_set<DataNodePtr> &visited, std::unordered_set<DataNodePtr> &recursionStack) {
        if (recursionStack.count(node)) return true;
        if (visited.count(node)) return false;

        visited.insert(node);
        recursionStack.insert(node);

        for (auto neighbor : m_dependents[node]) {
            if (dfs(std::get<DataNodePtr>(neighbor), visited, recursionStack)) return true;
        }

        recursionStack.erase(node);
        return false;
    }

    // Check for repeat dependencies
    template <NodeCC NodeType>
    bool hasRepeatDependencies(std::shared_ptr<NodeType> node, DataNodePtr target) {
        std::unordered_set<DataNodePtr> targetDependencies;
        collectDependencies(target, targetDependencies);

        std::unordered_set<DataNodePtr> visited;
        std::unordered_set<DataNodePtr> dependents;

        return std::any_of(m_dependents[node].begin(), m_dependents[node].end(),
                           [&](auto dependent) {
                               return checkDependency(std::get<DataNodePtr>(dependent), targetDependencies, visited);
                           });
    }

    // Collect all dependencies for a given node
    void collectDependencies(DataNodePtr node, std::unordered_set<DataNodePtr> &dependencies) {
        if (!node || dependencies.count(node)) return;
        dependencies.insert(node);

        for (auto neighbor : m_dependents[node]) {
            collectDependencies(std::get<DataNodePtr>(neighbor), dependencies);
        }
    }

    // Check if a node is part of a target's dependencies
    bool checkDependency(DataNodePtr node, const std::unordered_set<DataNodePtr> &targetDependencies, std::unordered_set<DataNodePtr> &visited) {
        if (visited.count(node)) return false;
        visited.insert(node);

        if (targetDependencies.count(node)) return true;

        return std::any_of(m_dependents[node].begin(), m_dependents[node].end(),
                           [&](auto neighbor) {
                               return checkDependency(std::get<DataNodePtr>(neighbor), targetDependencies, visited);
                           });
    }

    std::unordered_map<NodeVariant, std::unordered_map<NodeVariant, ObserverCallback, NodeVariantHash, NodeVariantEqual>, NodeVariantHash, NodeVariantEqual> m_observers;
    std::unordered_map<NodeVariant, std::unordered_set<NodeVariant, NodeVariantHash, NodeVariantEqual>, NodeVariantHash, NodeVariantEqual> m_dependents;
};

// Base class for all observer nodes
template <typename Derived>
class ObserverBase : public std::enable_shared_from_this<Derived> {
public:
    using SourceType = DataNode;
    ObserverBase(const ObserverBase &) = delete;
    ObserverBase &operator=(const ObserverBase &) = delete;
    ObserverBase(ObserverBase &&) = delete;
    ObserverBase &operator=(ObserverBase &&) = delete;
    // Constructor with an optional name
    ObserverBase(const std::string &name = "") :
        m_name(name) {
    }

    // Set and get the name of the observer
    void setName(const std::string &name) {
        m_name = name;
    }
    std::string getName() const {
        return m_name;
    }

    // Get a shared pointer to the derived class
    std::shared_ptr<Derived> getShared() {
        return this->shared_from_this();
    }

protected:
    void notifyObservers(bool changed = true) {
        notify(changed);
    }

    bool updateObservers(bool &repeat, std::function<void(bool)> &&f, auto &&...args) {
        auto shared_this = getShared();
        ObserverGraph::getInstance().resetNode(shared_this);
        if (!(ObserverGraph::getInstance().addObserver(repeat, f, shared_this, args.getShared()) && ...)) {
            ObserverGraph::getInstance().resetNode(shared_this);
            return false;
        }
        return true;
    }

    void updateOneObserver(bool &repeat, std::function<void(bool)> &&f, DataNodePtr node) {
        auto shared_this = getShared();
        ObserverGraph::getInstance().addObserver(repeat, f, shared_this, node);
    }

    void notify(bool changed) {
        for (auto fun : m_observers) {
            std::invoke(fun, changed);
        }
    }

private:
    friend class ObserverGraph;
    void addCb(const ObserverCallback &cb) {
        m_observers.emplace_back(cb);
    }

    void deleteCb(const ObserverCallback &cb) {
        m_observers.erase(std::remove(m_observers.begin(), m_observers.end(), cb), m_observers.end());
    }

    std::string m_name;
    std::vector<ObserverCallback> m_observers;
};

// ObserverDataNode handles data node-specific observers
class ObserverDataNode : public ObserverBase<ObserverDataNode> {
public:
    using SourceType = DataNode;
};

// ObserverActionNode handles action node-specific observers
class ObserverActionNode : public ObserverBase<ObserverActionNode> {
public:
    using SourceType = ActionNode;
};

// ObserverFieldNode handles field node-specific observers
class ObserverFieldNode : public ObserverBase<ObserverFieldNode> {
public:
    using SourceType = FieldNode;
};

} // namespace reaction

#endif // REACTION_OBSERVERNODE_H
