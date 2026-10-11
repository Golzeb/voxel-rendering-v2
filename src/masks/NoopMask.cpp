#include "NoopMask.h"

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

bool NoopMask::is_visible(glm::ivec3 position, VoxelData::Type voxel_value) const {
    return true;
}
