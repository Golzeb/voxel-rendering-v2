#include "NaiveMeshExtractor.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>

#include "Mesh.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

std::optional<Mesh> NaiveMeshExtractor::extract_mesh(IVoxelDataProvider::VoxelDataChunk const& chunk,
                                                     std::shared_ptr<ITransferFunction> transfer_function,
                                                     IMask const& mask) {
    Mesh mesh;
    tracked_umap<VoxelData::Type, std::uint32_t> colour_map;

    glm::ivec3 const size = chunk.data.size();

    VoxelData const& data = chunk.data;

    for (std::size_t x = 0; x < size.x; x++) {
        for (std::size_t y = 0; y < size.y; y++) {
            for (std::size_t z = 0; z < size.z; z++) {
                VoxelData::Type const voxel_value = data.get_voxel(glm::ivec3{x, y, z});
                glm::ivec3 const position{x, y, z};

                if (!mask.is_visible(position, voxel_value)) continue;

                std::uint32_t colour;

                if (colour_map.contains(voxel_value)) {
                    colour = colour_map[voxel_value];
                } else {
                    colour = mesh.colours.size();
                    mesh.colours.push_back(glm::vec4(transfer_function->colour(position, voxel_value), 1.0f));
                    mesh.values.push_back(voxel_value);
                    colour_map.insert_or_assign(voxel_value, colour);
                }

                mesh::add_cube(mesh, colour, position);
            }
        }
    }

    mesh.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.position));

    if (mesh.vertices.size() > 0) {
        return mesh;
    }

    return std::nullopt;
}
