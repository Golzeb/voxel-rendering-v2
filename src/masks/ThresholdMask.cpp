#include "ThresholdMask.h"

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

bool ThresholdMask::is_visible(glm::ivec3 position, VoxelData::Type voxel_value) const {
    return m_inverted ? (voxel_value > m_threshold) : (voxel_value < m_threshold);
}

ThresholdMask::ThresholdMask(VoxelData::Type threshold, bool inverted) {
    m_inverted = inverted;
    m_threshold = threshold;
}
