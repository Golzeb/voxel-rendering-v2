#pragma once
#include <cstddef>
#include <functional>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

constexpr std::size_t KILL_SWITCH = 22;

namespace talloc {
inline std::shared_ptr<std::size_t> current_memory() {
    static std::shared_ptr<std::size_t> current_memory = std::make_shared<std::size_t>();
    return current_memory;
}

inline std::shared_ptr<std::size_t> max_memory() {
    static std::shared_ptr<std::size_t> max_memory = std::make_shared<std::size_t>();
    return max_memory;
}

inline void reset() {
    *talloc::current_memory() = 0;
    *talloc::max_memory() = 0;
}
}  // namespace talloc

template <typename T>
class TrackingAllocator {
public:
    using value_type = T;

    TrackingAllocator() noexcept {}

    bool operator==(TrackingAllocator& other) const {
        return true;
    }

    template <typename U>
    TrackingAllocator(TrackingAllocator<U> const& other) {}

    value_type* allocate(std::size_t n) {
        void* data = ::operator new(n * sizeof(value_type));
        *talloc::current_memory() += n * sizeof(value_type);
        if (*talloc::current_memory() > std::size_t(KILL_SWITCH) * 1024 * 1024 * 1024)
            throw std::runtime_error("Memory usage higher than 22 GiB");
        if (*talloc::current_memory() > *talloc::max_memory()) *talloc::max_memory() = *talloc::current_memory();
        return static_cast<value_type*>(data);
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        *talloc::current_memory() -= n * sizeof(value_type);
        ::operator delete(p);
    }

private:
};

template <typename T>
using tracked_vector = std::vector<T, TrackingAllocator<T>>;

template <typename K, typename V>
using tracked_umap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, TrackingAllocator<std::pair<K const, V>>>;
