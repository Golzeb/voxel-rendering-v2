#pragma once
// clang-format off
#include "glad/gl.h"
#include "utils.h"
// clang-format on

#include <atomic>
#include <bit>
#include <cstddef>
#include <deque>
#include <format>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

// 28 GiB
constexpr std::size_t KILL_SWITCH = std::size_t{28} * std::size_t{1024} * std::size_t{1024} * std::size_t{1024};

namespace talloc {
inline std::size_t& current_gpu_memory() {
    static std::size_t current_gpu_memory = 0;
    return current_gpu_memory;
}

inline void glNamedBufferData(GLuint buffer, GLsizeiptr size, void const* data, GLenum usage) {
    current_gpu_memory() += std::bit_cast<std::size_t>(size);
    ::glNamedBufferData(buffer, size, data, usage);
}

inline std::atomic<std::size_t>& current_memory() {
    static std::atomic<std::size_t> current_memory = 0;
    return current_memory;
}

inline std::atomic<std::size_t>& max_memory() {
    static std::atomic<std::size_t> max_memory = 0;
    return max_memory;
}

inline void reset() {
    current_gpu_memory() = 0;
    current_memory() = 0;
    max_memory() = 0;
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
        talloc::current_memory() += n * sizeof(value_type);
        if (talloc::current_memory() > KILL_SWITCH)
            throw std::runtime_error(
                std::format("Memory usage higher than {}", utils::human_readable_size(KILL_SWITCH)));
        if (talloc::current_memory() > talloc::max_memory()) talloc::max_memory().store(talloc::current_memory());
        return static_cast<value_type*>(data);
    }

    void deallocate(value_type* p, std::size_t n) noexcept {
        talloc::current_memory() -= n * sizeof(value_type);
        ::operator delete(p);
    }

private:
};

template <typename T>
using tracked_vector = std::vector<T, TrackingAllocator<T>>;

template <typename K, typename V>
using tracked_umap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, TrackingAllocator<std::pair<K const, V>>>;

template <typename T>
using tracked_deque = std::deque<T, TrackingAllocator<T>>;
