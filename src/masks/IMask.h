#pragma once

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

class IMask {
public:
    virtual ~IMask() = default;

    virtual bool is_visible(glm::ivec3 position, VoxelData::Type voxel_value) const = 0;
};
