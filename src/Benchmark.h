#pragma once
#include <cstddef>
#include <vector>

#include "Camera.h"
#include "glm/ext/vector_float3.hpp"
#include "utils.h"

class ApplicationState;

class Benchmark {
public:
    struct Point {
        std::size_t length;
        glm::vec3 look_at;
        glm::vec3 position;
    };

    struct Stats {
        float average_frame_time;
        float average_load_time;
        float vram_usage;
        float ram_usage;
    };

public:
    Benchmark() = default;
    ~Benchmark() = default;

    void start(ApplicationState& appstate, std::vector<Point> const& points);
    void run(Camera& camera);

    bool is_running();
    Stats stats();

private:
    std::vector<Point> m_points;
    utils::time_point_t m_start;
    std::size_t m_current_frame;
    std::size_t m_frame_total;
    std::size_t m_index;
    bool m_running = false;

    Stats m_stats;
};
