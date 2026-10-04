#include "ValueColourMapping.h"

#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

ValueColourMapping::ValueColourMapping() {
    m_default_colour = glm::vec3(1.0);
    m_colour_map = {};
}

void ValueColourMapping::set_default_colour(glm::vec3 const& new_colour) {
    m_default_colour = new_colour;
}

void ValueColourMapping::set_colour(VoxelData::Type value, glm::vec3 const& colour) {
    m_colour_map.insert_or_assign(value, colour);
}

glm::vec3 ValueColourMapping::colour(glm::ivec3 voxel_position, VoxelData::Type voxel_value) {
    if (m_colour_map.contains(voxel_value)) {
        return m_colour_map.at(voxel_value);
    }

    return m_default_colour;
}

bool ValueColourMapping::controls() {
    return false;
}
