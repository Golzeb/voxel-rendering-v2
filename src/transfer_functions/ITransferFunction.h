#pragma once
#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

class ITransferFunction {
public:
    virtual ~ITransferFunction() = default;
    virtual glm::vec3 colour(glm::ivec3 voxel_position, VoxelData::Type voxel_value) = 0;
};
