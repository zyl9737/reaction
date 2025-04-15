#ifndef OBSERVER_CALLBACK_HASH_H
#define OBSERVER_CALLBACK_HASH_H

#include "reaction/concept.h"
#include <functional>
#include <utility>
#include <atomic>
#include <memory>
#include <variant>

namespace reaction {
class ObserverCallback {
public:
    using CallbackType = std::function<void(bool)>;

    ObserverCallback(const CallbackType &callback = nullptr) :
        m_id(generateId()), m_callback(callback) {
    }

    std::uint64_t getId() const {
        return m_id;
    }

    void operator()(bool flag) const {
        if (m_callback) {
            m_callback(flag);
        }
    }

    bool operator==(const ObserverCallback &other) const {
        return m_id == other.m_id;
    }

private:
    std::uint64_t m_id;
    CallbackType m_callback;

    static std::uint64_t generateId() {
        static std::atomic<std::uint64_t> counter{0};
        return ++counter;
    }
};

// Forward declarations for different node types
struct DataNode {};
struct ActionNode {};
struct FieldNode {};

using DataNodePtr = std::shared_ptr<ObserverDataNode>;
using ActionNodePtr = std::shared_ptr<ObserverActionNode>;
using FieldNodePtr = std::shared_ptr<ObserverFieldNode>;

using NodeVariant = std::variant<DataNodePtr, ActionNodePtr>;

struct NodeVariantHash {
    std::size_t operator()(const NodeVariant &node) const {
        return std::visit([](auto&& ptr) -> std::size_t {
            return std::hash<typename std::decay<decltype(ptr)>::type>()(ptr);
        }, node);
    }
};

struct NodeVariantEqual {
    bool operator()(const NodeVariant &a, const NodeVariant &b) const {
        return a.index() == b.index() &&
        std::visit([](auto &&ptr1, auto &&ptr2) -> bool {
            using T1 = std::decay_t<decltype(ptr1)>;
            using T2 = std::decay_t<decltype(ptr2)>;
            if constexpr (std::is_same_v<T1, T2>) {
                return ptr1 == ptr2;
            } else {
                return false;
            }
        }, a, b);
    }
};

} // namespace reaction
#endif // OBSERVER_CALLBACK_HASH_H
