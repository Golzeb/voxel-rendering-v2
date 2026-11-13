#include "FaceCulledGreedyMeshExtractor.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
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
#include "taskflow/core/executor.hpp"
#include "taskflow/taskflow.hpp"
#include "transfer_functions/ITransferFunction.h"

std::optional<Mesh> FaceCulledGreedyMeshExtractor::extract_mesh(IVoxelDataProvider::VoxelDataChunk const& chunk,
                                                                std::shared_ptr<ITransferFunction> transfer_function,
                                                                std::shared_ptr<IMask> mask) {
    std::array<std::int32_t, 3> data_size = {
        chunk.data.size().x,
        chunk.data.size().y,
        chunk.data.size().z,
    };

    Mesh mesh;
    tracked_umap<VoxelData::Type, std::uint32_t> colour_map;

    std::size_t index = 0;

    std::mutex mutex;
    tf::Executor executor;

    for (std::size_t dim = 0; dim < 3; dim++) {
        executor.silent_async([&, dim]() {
            std::int32_t u = (dim + 1) % 3;
            std::int32_t v = (dim + 2) % 3;

            std::array<std::int32_t, 3> current_pos{}, offset{};
            offset[dim] = 1;

            std::size_t mask_size = 0;

            switch (dim) {
                case 0:  // X
                    mask_size = chunk.data.size().z * chunk.data.size().y;
                    break;
                case 1:  // Y
                    mask_size = chunk.data.size().z * chunk.data.size().x;
                    break;
                case 2:  // Z
                    mask_size = chunk.data.size().x * chunk.data.size().y;
                    break;
            }

            tracked_vector<bool> mesh_mask;
            mesh_mask.resize(mask_size, false);

            tracked_vector<bool> flip_mask;
            flip_mask.resize(mask_size, false);

            tracked_vector<VoxelData::Type> voxel_types;
            voxel_types.resize(mask_size);

            // Generowanie maski widocznych wokseli
            for (current_pos[dim] = -1; current_pos[dim] < data_size[dim];) {
                std::int32_t n = 0;
                for (current_pos[v] = 0; current_pos[v] < data_size[v]; current_pos[v]++) {
                    for (current_pos[u] = 0; current_pos[u] < data_size[u]; current_pos[u]++) {
                        bool current, compare;

                        if (current_pos[dim] < 0) {
                            current = false;
                        } else {
                            VoxelData::Type voxel_current =
                                chunk.data.get_voxel(glm::ivec3{current_pos[0], current_pos[1], current_pos[2]});
                            current = mask->is_visible(glm::vec3{current_pos[0], current_pos[1], current_pos[2]},
                                                       voxel_current);
                            if (current) voxel_types[n] = voxel_current;
                        }

                        if (current_pos[dim] == data_size[dim] - 1) {
                            compare = false;
                        } else {
                            VoxelData::Type voxel_compare = chunk.data.get_voxel(glm::ivec3{
                                current_pos[0] + offset[0], current_pos[1] + offset[1], current_pos[2] + offset[2]});
                            compare = mask->is_visible(glm::vec3{current_pos[0] + offset[0], current_pos[1] + offset[1],
                                                                 current_pos[2] + offset[2]},
                                                       voxel_compare);
                            if (compare) voxel_types[n] = voxel_compare;
                        }

                        mesh_mask[n] = current != compare;
                        flip_mask[n] = !compare;
                        n += 1;
                    }
                }

                current_pos[dim] += 1;
                n = 0;

                for (std::size_t j = 0; j < data_size[v]; j++) {
                    for (std::size_t i = 0; i < data_size[u];) {
                        if (mesh_mask[n]) {
                            std::size_t quad_width = 0, quad_height = 0;
                            for (quad_width = 1; i + quad_width < data_size[u] && mesh_mask[n + quad_width] &&
                                                 flip_mask[n] == flip_mask[n + quad_width] &&
                                                 voxel_types[n] == voxel_types[n + quad_width];
                                 quad_width++);

                            bool done = false;
                            for (quad_height = 1; j + quad_height < data_size[v]; quad_height++) {
                                for (std::size_t k = 0; k < quad_width; k++) {
                                    if (!mesh_mask[n + k + quad_height * data_size[u]] ||
                                        flip_mask[n] != flip_mask[n + k + quad_height * data_size[u]] ||
                                        voxel_types[n] != voxel_types[n + k + quad_height * data_size[u]]) {
                                        done = true;
                                        break;
                                    }
                                }
                                if (done) break;
                            }

                            current_pos[v] = j;
                            current_pos[u] = i;

                            std::array<std::int32_t, 3> du{}, dv{};
                            du[u] = quad_width;
                            dv[v] = quad_height;

                            std::uint32_t colour;

                            {
                                std::lock_guard lock(mutex);
                                if (colour_map.contains(voxel_types[n])) {
                                    colour = colour_map[voxel_types[n]];
                                } else {
                                    colour = mesh.colours.size();
                                    mesh.colours.push_back(
                                        glm::vec4(transfer_function->colour(
                                                      {current_pos[0], current_pos[1], current_pos[2]}, voxel_types[n]),
                                                  1.0f));
                                    mesh.values.push_back(voxel_types[n]);
                                    colour_map.insert_or_assign(voxel_types[n], colour);
                                }
                            }

                            if (!flip_mask[n]) {
                                std::lock_guard lock(mutex);
                                mesh::add_quad(
                                    mesh, colour,
                                    {
                                        glm::vec3{current_pos[0], current_pos[1], current_pos[2]},
                                        glm::vec3{current_pos[0] + du[0], current_pos[1] + du[1],
                                                  current_pos[2] + du[2]},
                                        glm::vec3{current_pos[0] + du[0] + dv[0], current_pos[1] + du[1] + dv[1],
                                                  current_pos[2] + du[2] + dv[2]},
                                        glm::vec3{current_pos[0] + dv[0], current_pos[1] + dv[1],
                                                  current_pos[2] + dv[2]},
                                    });
                            } else {
                                std::lock_guard lock(mutex);
                                mesh::add_quad(
                                    mesh, colour,
                                    {
                                        glm::vec3{current_pos[0], current_pos[1], current_pos[2]},
                                        glm::vec3{current_pos[0] + dv[0], current_pos[1] + dv[1],
                                                  current_pos[2] + dv[2]},
                                        glm::vec3{current_pos[0] + du[0] + dv[0], current_pos[1] + du[1] + dv[1],
                                                  current_pos[2] + du[2] + dv[2]},
                                        glm::vec3{current_pos[0] + du[0], current_pos[1] + du[1],
                                                  current_pos[2] + du[2]},
                                    });
                            }

                            for (std::size_t l = 0; l < quad_height; l++) {
                                for (std::size_t k = 0; k < quad_width; k++) {
                                    mesh_mask[n + k + l * data_size[u]] = false;
                                }
                            }

                            i += quad_width;
                            n += quad_width;

                        } else {
                            i += 1;
                            n += 1;
                        }
                    }
                }
            }
        });
    }

    executor.wait_for_all();

    mesh.model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.position));

    if (mesh.vertices.size() > 0) {
        return mesh;
    }

    return std::nullopt;
}
