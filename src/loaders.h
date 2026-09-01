#pragma once
#include <string_view>

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

namespace gv {
VoxelData read_raw(std::string_view path, glm::ivec3 const& size);
VoxelData read_vl32(std::string_view path, glm::ivec3 const& size);
};  // namespace gv
