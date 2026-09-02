#pragma once

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

class Mask {
public:
    virtual ~Mask() = default;

    virtual bool is_visible(glm::ivec3 const& position, VoxelData::Type voxel_value) const = 0;
};
