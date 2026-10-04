#include "PiecewiseColourMapping.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <vector>

#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"

void PiecewiseColourMapping::add_point(glm::vec3 const& colour, VoxelData::Type point) {
    for (std::size_t i = 0; i < m_points.size(); i++) {
        if (m_points[i].point == point) return;
    }

    m_points.push_back({.point = point, .colour = colour});
}

glm::vec3 PiecewiseColourMapping::colour(glm::ivec3 voxel_position, VoxelData::Type voxel_value) {
    std::size_t closest_lower = 0;
    std::optional<std::size_t> closest_higher = std::nullopt;

    std::int32_t closest_higher_distance = std::numeric_limits<std::int32_t>::max();
    std::int32_t closest_lower_distance = std::numeric_limits<std::int32_t>::max();

    if (m_points.size() == 0) {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    if (m_points.size() == 1) {
        return m_points.begin()->colour;
    }

    for (std::size_t i = 0; i < m_points.size(); i++) {
        if (m_points[i].point == voxel_value) return m_points[i].colour;

        std::int32_t distance = std::abs(m_points[i].point - voxel_value);
        if (m_points[i].point > voxel_value && distance < closest_higher_distance) {
            closest_higher = i;
            closest_higher_distance = distance;
        }

        if (m_points[i].point < voxel_value && distance < closest_lower_distance) {
            closest_lower = i;
            closest_lower_distance = distance;
        }
    }

    if (!closest_higher.has_value()) {
        return m_points[closest_lower].colour;
    }

    if (closest_higher == closest_lower) {
        return m_points[closest_lower].colour;
    }

    Point const& higher = m_points[closest_higher.value()];
    Point const& lower = m_points[closest_lower];

    std::int32_t distance = higher.point - lower.point;
    float alpha = (voxel_value - lower.point) / float(distance);

    return lower.colour * (1.0f - alpha) + higher.colour * alpha;
}

bool PiecewiseColourMapping::controls() {
    static VoxelData::Type const min_value = std::numeric_limits<VoxelData::Type>::min();
    static VoxelData::Type const max_value = std::numeric_limits<VoxelData::Type>::max();

    static bool previous_updated = false;
    static bool updated = false;

    previous_updated = updated;
    updated = false;

    if (ImGui::Button("Sort colours")) {
        std::sort(m_points.begin(), m_points.end(),
                  [](Point const& left, Point const& right) { return left.point < right.point; });
        updated |= true;
    }

    if (ImGui::Button("Add Colour")) {
        m_points.push_back({
            .point = 0,
            .colour = glm::vec3(0.5f, 0.5f, 0.5f),
        });
        updated |= true;
    }

    std::vector<std::size_t> remove_list;
    for (std::size_t i = 0; i < m_points.size(); i++) {
        ImGui::PushID(i);
        std::string name = std::format("Colour #{}", i);
        if (ImGui::CollapsingHeader(name.c_str())) {
            if (ImGui::Button("Remove colour")) {
                remove_list.push_back(i - remove_list.size());
                updated |= true;
            }
            updated |= ImGui::ColorPicker3("", glm::value_ptr(m_points[i].colour));
            updated |= ImGui::SliderScalar("Point", ImGuiDataType_S16, &m_points[i].point, &min_value, &max_value);
        }
        ImGui::PopID();
    }

    for (auto index : remove_list) {
        m_points.erase(m_points.begin() + index);
    }

    return previous_updated && !updated;
}
