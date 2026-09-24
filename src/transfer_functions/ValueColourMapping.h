#pragma once
#include <unordered_map>

#include "ITransferFunction.h"
#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

class ValueColourMapping : public ITransferFunction {
public:
    ValueColourMapping();
    ~ValueColourMapping() = default;

    void set_default_colour(glm::vec3 const& new_colour);
    void set_colour(VoxelData::Type value, glm::vec3 const& colour);

    glm::vec3 colour(glm::ivec3 voxel_position, VoxelData::Type voxel_value) override;

private:
    glm::vec3 m_default_colour;
    std::unordered_map<VoxelData::Type, glm::vec3> m_colour_map;
};
