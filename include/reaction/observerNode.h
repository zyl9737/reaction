#ifndef REACTION_OBSERVERNODE_H
#define REACTION_OBSERVERNODE_H

#include <unordered_set>
#include <unordered_map>
#include <memory>
#include "reaction/exception.h"  // Ensure to include exception handling

namespace reaction {

    // Forward declaration for ObserverNode
    class ObserverNode;

    using ObserverNodePtr = std::shared_ptr<ObserverNode>;

    // ObserverGraph class for managing nodes and their dependencies
    class ObserverGraph {
    public:
        static ObserverGraph &getInstance() {
            static ObserverGraph instance;
            return instance;
        }

        void addNode(ObserverNodePtr node);
        bool addDataObserver(ObserverNodePtr source, ObserverNodePtr target);
        bool addActionObserver(ObserverNodePtr source, ObserverNodePtr target);
        void moveNode(ObserverNodePtr dst, ObserverNodePtr src);
        void resetDataNode(ObserverNodePtr node);
        void resetActionNode(ObserverNodePtr node);
        void closeNode(ObserverNodePtr node);
        void deleteNode(ObserverNodePtr node);
        std::unordered_set<ObserverNodePtr> getDataObserverList(ObserverNodePtr node);
        std::unordered_set<ObserverNodePtr> getActionObserverList(ObserverNodePtr node);

    private:
        ObserverGraph() {}

        std::unordered_map<ObserverNodePtr, std::unordered_set<ObserverNodePtr>> m_dataDependents;
        std::unordered_map<ObserverNodePtr, std::unordered_set<ObserverNodePtr>> m_dataObservers;
        std::unordered_map<ObserverNodePtr, std::unordered_set<ObserverNodePtr>> m_actionObservers;
        std::unordered_map<ObserverNodePtr, std::unordered_set<ObserverNodePtr>> m_actionDependents;

        bool hasCycle(ObserverNodePtr source, ObserverNodePtr target);
        bool dfs(ObserverNodePtr node, std::unordered_set<ObserverNodePtr> &visited, ObserverNodePtr target);
        bool hasDependency(ObserverNodePtr source, ObserverNodePtr target);
        bool dfsForIndirect(ObserverNodePtr node, std::unordered_set<ObserverNodePtr> &visited, ObserverNodePtr target);
    };

    // ObserverNode class for managing individual nodes
    class ObserverNode : public std::enable_shared_from_this<ObserverNode> {
        friend class ObserverGraph;

    public:
        ObserverNode(bool isAction = false) : m_isAction(isAction) {}
        virtual ~ObserverNode() {}

        void setName(const std::string &name) { m_name = name; }
        std::string getName() { return m_name; }

    protected:
        std::shared_ptr<ObserverNode> getShared() { return shared_from_this(); }
        virtual void valueChanged(bool changed = true) {}
        bool isAction() { return m_isAction; }

        void notifyObservers(bool changed = true) {
            for (auto &observer : ObserverGraph::getInstance().getDataObserverList(shared_from_this())) {
                observer->valueChanged(changed);
            }
            for (auto &observer : ObserverGraph::getInstance().getActionObserverList(shared_from_this())) {
                observer->valueChanged(changed);
            }
        }

        void updateObservers(auto &&...args) {
            std::shared_ptr<ObserverNode> shared_this = shared_from_this();

            if (m_isAction) {
                ObserverGraph::getInstance().resetActionNode(shared_this);
                (ObserverGraph::getInstance().addActionObserver(shared_this, args->getShared()), ...);
            } else {
                ObserverGraph::getInstance().resetDataNode(shared_this);
                (ObserverGraph::getInstance().addDataObserver(shared_this, args->getShared()), ...);
            }
        }

    private:
        std::string m_name;
        bool m_isAction = false;
    };

    // Implementation of ObserverGraph methods

    inline void ObserverGraph::addNode(ObserverNodePtr node) {
        m_dataDependents[node] = std::unordered_set<ObserverNodePtr>();
        m_dataObservers[node] = std::unordered_set<ObserverNodePtr>();
        m_actionObservers[node] = std::unordered_set<ObserverNodePtr>();
        m_actionDependents[node] = std::unordered_set<ObserverNodePtr>();
    }

    inline bool ObserverGraph::addDataObserver(ObserverNodePtr source, ObserverNodePtr target) {
        if (source == target) {
            throw Exception{"cannot observe self, node = {}.", source->getName()};
        }

        if (hasDependency(source, target)) {
            return false;
        }

        if (hasCycle(source, target)) {
            throw Exception{"detect cycle dependency, node = {}. cycle dependent = {}", source->getName(), target->getName()};
        }

        m_dataDependents[source].insert(target);
        m_dataObservers[target].insert(source);
        return true;
    }

    inline bool ObserverGraph::addActionObserver(ObserverNodePtr source, ObserverNodePtr target) {
        if (source == target) {
            throw Exception{"cannot observe self, node = {}.", source->getName()};
        }

        if (hasDependency(source, target)) {
            return false;
        }

        m_actionDependents[source].insert(target);
        m_actionObservers[target].insert(source);
        return true;
    }

    inline void ObserverGraph::moveNode(ObserverNodePtr dst, ObserverNodePtr src) {
        m_dataDependents[dst] = std::move(m_dataDependents[src]);
        m_dataObservers[dst] = std::move(m_dataObservers[src]);
        m_actionObservers[dst] = std::move(m_actionObservers[src]);
        m_actionDependents[dst] = std::move(m_actionDependents[src]);

        for (auto &[node, dependents] : m_dataDependents) {
            if (dependents.erase(src)) {
                dependents.insert(dst);
            }
        }
        for (auto &[node, dependents] : m_actionDependents) {
            if (dependents.erase(src)) {
                dependents.insert(dst);
            }
        }

        for (auto &[node, observers] : m_dataObservers) {
            if (observers.erase(src)) {
                observers.insert(dst);
            }
        }
        for (auto &[node, observers] : m_actionObservers) {
            if (observers.erase(src)) {
                observers.insert(dst);
            }
        }

        deleteNode(src);
    }

    inline void ObserverGraph::resetDataNode(ObserverNodePtr node) {
        for (auto dep : m_dataDependents[node]) {
            m_dataObservers[dep].erase(node);
        }
        m_dataDependents[node].clear();
    }

    inline void ObserverGraph::resetActionNode(ObserverNodePtr node) {
        for (auto dep : m_actionDependents[node]) {
            m_actionObservers[dep].erase(node);
        }
        m_actionDependents[node].clear();
    }

    inline void ObserverGraph::closeNode(ObserverNodePtr node) {
        if (node->isAction()) {
            for (auto dep : m_actionDependents[node]) {
                m_actionObservers[dep].erase(node);
            }
        } else {
            for (auto dep : m_dataDependents[node]) {
                m_dataObservers[dep].erase(node);
            }
        }

        for (auto ob : m_dataObservers[node]) {
            m_dataObservers[node].erase(ob);
            closeNode(ob);
        }

        for (auto ob : m_actionObservers[node]) {
            m_actionObservers[node].erase(ob);
            closeNode(ob);
        }

        deleteNode(node);
    }

    inline void ObserverGraph::deleteNode(ObserverNodePtr node) {
        m_dataDependents.erase(node);
        m_dataObservers.erase(node);
        m_actionObservers.erase(node);
        m_actionDependents.erase(node);
    }

    inline std::unordered_set<ObserverNodePtr> ObserverGraph::getDataObserverList(ObserverNodePtr node) {
        return m_dataObservers[node];
    }

    inline std::unordered_set<ObserverNodePtr> ObserverGraph::getActionObserverList(ObserverNodePtr node) {
        return m_actionObservers[node];
    }

    inline bool ObserverGraph::hasCycle(ObserverNodePtr source, ObserverNodePtr target) {
        m_dataDependents[source].insert(target);
        m_dataObservers[target].insert(source);

        std::unordered_set<ObserverNodePtr> visited;
        if (dfs(source, visited, target)) {
            m_dataDependents[source].erase(target);
            m_dataObservers[target].erase(source);
            return true;
        }

        m_dataDependents[source].erase(target);
        m_dataObservers[target].erase(source);
        return false;
    }

    inline bool ObserverGraph::dfs(ObserverNodePtr node, std::unordered_set<ObserverNodePtr> &visited, ObserverNodePtr target) {
        if (visited.count(node)) {
            return node == target;
        }

        visited.insert(node);
        for (auto neighbor : m_dataDependents[node]) {
            if (dfs(neighbor, visited, target)) {
                return true;
            }
        }
        visited.erase(node);
        return false;
    }

    inline bool ObserverGraph::hasDependency(ObserverNodePtr source, ObserverNodePtr target) {
        std::unordered_set<ObserverNodePtr> visited;
        return dfsForIndirect(source, visited, target);
    }

    inline bool ObserverGraph::dfsForIndirect(ObserverNodePtr node, std::unordered_set<ObserverNodePtr> &visited, ObserverNodePtr target) {
        if (visited.count(node)) {
            return false;
        }

        visited.insert(node);

        if (node == target) {
            return true;
        }

        auto dependents = node->isAction() ? m_actionDependents : m_dataDependents;

        for (auto neighbor : dependents[node]) {
            if (dfsForIndirect(neighbor, visited, target)) {
                return true;
            }
        }

        return false;
    }

} // namespace reaction

#endif // REACTION_OBSERVERNODE_H
