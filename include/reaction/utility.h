/*
 * Copyright (c) 2025 Lummy
 *
 * This software is released under the MIT License.
 * See the LICENSE file in the project root for full details.
 */

#ifndef REACTION_UTILITY_H
#define REACTION_UTILITY_H

#include <atomic>
#include <unordered_set>
#include <unordered_map>
namespace reaction {

class UniqueID {
public:
    UniqueID() :
        id_(generate()) {
    }

    operator uint64_t() {
        return id_;
    }

    bool operator==(const UniqueID &other) const {
        return id_ == other.id_;
    }

private:
    uint64_t id_;

    static uint64_t generate() {
        static std::atomic<uint64_t> counter{0};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
    friend struct std::hash<UniqueID>;
};
} // namespace reaction

namespace std {
template <>
struct hash<reaction::UniqueID> {
    std::size_t operator()(const reaction::UniqueID &uid) const noexcept {
        return std::hash<uint64_t>{}(uid.id_);
    }
};

} // namespace std

#endif // REACTION_UTILITY_H
