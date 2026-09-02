#include "loaders.h"

#include <bit>
#include <cstdint>
#include <fstream>
#include <ios>
#include <memory>
#include <string_view>
#include <tuple>

#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "transfer_functions/ITransferFunction.h"
#include "transfer_functions/ValueColourMapping.h"

namespace loaders {
VoxelData read_raw(std::string_view path, glm::ivec3 const& size) {
    std::ifstream file;
    file.open(path.data(), std::ios::in | std::ios::binary);

    VoxelData data(size);

    glm::ivec3 current_position{};

    std::int16_t voxel_value;
    while (file.read((char*)&voxel_value, 2)) {
        if constexpr (std::endian::native != std::endian::little) {
            voxel_value = std::byteswap(voxel_value);
        }

        data.set_voxel(current_position, (std::int32_t)voxel_value + 32768);

        current_position.x += 1;

        current_position.y += current_position.x / size.x;
        current_position.x %= size.x;

        current_position.z += current_position.y / size.y;
        current_position.y %= size.y;
    }

    return data;
}

std::tuple<VoxelData, std::shared_ptr<ITransferFunction>> read_vl32(std::string_view path, glm::ivec3 const& size) {
    struct Voxel {
        std::int32_t x, y, z;
        std::uint32_t a : 8;
        std::uint32_t r : 8;
        std::uint32_t g : 8;
        std::uint32_t b : 8;
    };

    std::ifstream file;
    file.open(path.data(), std::ios::in | std::ios::binary);

    Voxel voxel;
    auto mapping = std::make_shared<ValueColourMapping>();

    VoxelData data(size);
    while (file.read((char*)&voxel, sizeof(voxel))) {
        if constexpr (std::endian::native != std::endian::big) {
            voxel.x = std::byteswap(voxel.x);
            voxel.y = std::byteswap(voxel.y);
            voxel.z = std::byteswap(voxel.z);
        }

        VoxelData::Type const voxel_value = (std::int16_t((std::uint8_t)voxel.r) >> 3) << 11 |
                                            (std::int16_t((std::uint8_t)voxel.g) >> 2) << 5 |
                                            (std::int16_t((std::uint8_t)voxel.b) >> 3);

        mapping->set_colour(voxel_value, glm::vec3{voxel.r / 255.0f, voxel.g / 255.0f, voxel.b / 255.0f});
        data.set_voxel(glm::ivec3{voxel.x, voxel.y, voxel.z}, voxel_value);
    }

    return {
        data,
        mapping,
    };
}

}  // namespace loaders
