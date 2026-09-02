#pragma once
#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"
#include "masks/Mask.h"

class ThresholdMask : public Mask {
public:
    bool is_visible(glm::ivec3 const& position, VoxelData::Type voxel_value) const override;

    ThresholdMask(VoxelData::Type threshold, bool inverted = false);
    ~ThresholdMask() override = default;

private:
    bool m_inverted;
    VoxelData::Type m_threshold;
};
