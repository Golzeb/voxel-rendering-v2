#include "NaiveMeshExtractor.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "Mesh.h"
#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "masks/Mask.h"
#include "transfer_functions/ITransferFunction.h"

void add_quad(Mesh& mesh, std::uint32_t colour, std::array<glm::vec3, 4> corners) {
    mesh.vertices.push_back({corners[2], colour});
    mesh.vertices.push_back({corners[1], colour});
    mesh.vertices.push_back({corners[0], colour});
    mesh.vertices.push_back({corners[0], colour});
    mesh.vertices.push_back({corners[3], colour});
    mesh.vertices.push_back({corners[2], colour});
}

void add_cube(Mesh& mesh, std::uint32_t colour, glm::vec3 position) {
    glm::vec3 const position000 = position;
    glm::vec3 const position001 = position + glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 const position010 = position + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 const position011 = position + glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 const position100 = position + glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 const position101 = position + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 const position110 = position + glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 const position111 = position + glm::vec3(1.0f, 1.0f, 1.0f);

    add_quad(mesh, colour, {position000, position100, position110, position010});
    add_quad(mesh, colour, {position001, position000, position010, position011});
    add_quad(mesh, colour, {position100, position101, position111, position110});
    add_quad(mesh, colour, {position101, position001, position011, position111});
    add_quad(mesh, colour, {position100, position000, position001, position101});
    add_quad(mesh, colour, {position010, position110, position111, position011});
}

std::optional<Mesh> NaiveMeshExtractor::extract_mesh(IVoxelDataProvider::VoxelDataChunk const& chunk,
                                                     std::shared_ptr<ITransferFunction> transfer_function,
                                                     Mask const& mask) {
    Mesh mesh;
    std::unordered_map<VoxelData::Type, std::uint32_t> colour_map;

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

                add_cube(mesh, colour, position);
            }
        }
    }

    mesh.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.position));

    if (mesh.vertices.size() > 0) {
        return mesh;
    }

    return std::nullopt;
}
