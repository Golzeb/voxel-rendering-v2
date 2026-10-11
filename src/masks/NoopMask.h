#pragma once
#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"
#include "masks/IMask.h"

class NoopMask : public IMask {
public:
    bool is_visible(glm::ivec3 position, VoxelData::Type voxel_value) const override;

    NoopMask() = default;
    ~NoopMask() override = default;
};
