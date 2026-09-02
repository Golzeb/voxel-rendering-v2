#pragma once
#include <memory>
#include <string_view>
#include <tuple>

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"
#include "transfer_functions/ITransferFunction.h"

namespace loaders {
VoxelData read_raw(std::string_view path, glm::ivec3 const& size);
std::tuple<VoxelData, std::shared_ptr<ITransferFunction>> read_vl32(std::string_view path, glm::ivec3 const& size);
};  // namespace loaders
