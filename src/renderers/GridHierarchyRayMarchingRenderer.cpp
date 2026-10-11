#include "GridHierarchyRayMarchingRenderer.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "config.h"
#include "data_providers/ChunkedDataProvider.h"
#include "data_providers/FullDataProvider.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"
#include "glm/vector_relational.hpp"
#include "masks/IMask.h"
#include "taskflow/algorithm/pipeline.hpp"
#include "taskflow/core/executor.hpp"
#include "taskflow/core/taskflow.hpp"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

constexpr std::uint32_t SCALE = 2;

struct Layer {
    glm::ivec3 size;
    tracked_vector<std::uint32_t> data;

    void init(glm::ivec3 previous_size) {
        size = glm::ceil(glm::vec3(previous_size) / (float)SCALE);

        data.resize(size.x * size.y * size.z, 0);
    }

    void set(glm::ivec3 position) {
        glm::ivec3 new_position = position / (std::int32_t)SCALE;

        std::size_t const index = new_position.x + new_position.y * size.x + new_position.z * size.x * size.y;

        data[index >> 5] |= (1 << (index & 0x1F));
    }

    bool get(glm::ivec3 position) {
        std::size_t const index = position.x + position.y * size.x + position.z * size.x * size.y;

        return (data[index >> 5] & (1 << (index & 0x1F)));
    }

    Layer next_layer() {
        Layer next{};
        next.init(size);

        for (std::size_t x = 0; x < size.x; x++) {
            for (std::size_t y = 0; y < size.y; y++) {
                for (std::size_t z = 0; z < size.z; z++) {
                    if (get(glm::ivec3{x, y, z})) next.set(glm::ivec3{x, y, z});
                }
            }
        }

        return next;
    }
};

GridHierarchyRayMarchingRenderer::GridHierarchyRayMarchingRenderer(Mode const& mode) {
    m_mode = mode;
}

void GridHierarchyRayMarchingRenderer::init() {
    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source;
        if (m_mode.chunked) {
            fragment_source = utils::load_shader("assets/chunked_grid_hierarchy_raymarch.frag");
        } else {
            fragment_source = utils::load_shader("assets/grid_hierarchy_raymarch.frag");
        }
        std::string vertex_source = utils::load_shader("assets/base_raymarch.vert");

        char const* c_fragment_source = fragment_source.c_str();
        char const* c_vertex_source = vertex_source.c_str();

        glShaderSource(fragment_shader, 1, &c_fragment_source, nullptr);
        glShaderSource(vertex_shader, 1, &c_vertex_source, nullptr);

        glCompileShader(fragment_shader);
        utils::check_shader_compilation_errors(fragment_shader);

        glCompileShader(vertex_shader);
        utils::check_shader_compilation_errors(vertex_shader);

        glAttachShader(m_shader_program, fragment_shader);
        glAttachShader(m_shader_program, vertex_shader);

        glLinkProgram(m_shader_program);

        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
    }

    glCreateBuffers(1, &m_vbo);

    glCreateVertexArrays(1, &m_vao);
    glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, 2 * sizeof(float));

    glVertexArrayAttribFormat(m_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(m_vao, 0, 0);
    glEnableVertexArrayAttrib(m_vao, 0);

    if (m_mode.chunked) {
        glCreateBuffers(1, &m_depth_buffer);
    }
}

void GridHierarchyRayMarchingRenderer::deinit() {
    glDeleteProgram(m_shader_program);

    glDeleteBuffers(1, &m_vbo);

    for (auto const& chunk : m_chunks) {
        glDeleteBuffers(1, &chunk.layer_descriptors_buffer);
        glDeleteBuffers(1, &chunk.layer_data_buffer);
        glDeleteBuffers(1, &chunk.voxel_data_buffer);
        glDeleteBuffers(1, &chunk.colour_mapping_buffer);
    }

    glDeleteVertexArrays(1, &m_vao);
}

void GridHierarchyRayMarchingRenderer::update_data(VoxelData& data,
                                                   std::shared_ptr<ITransferFunction> transfer_function,
                                                   std::shared_ptr<IMask> mask) {
    for (auto const& chunk : m_chunks) {
        glDeleteBuffers(1, &chunk.layer_descriptors_buffer);
        glDeleteBuffers(1, &chunk.layer_data_buffer);
        glDeleteBuffers(1, &chunk.voxel_data_buffer);
        glDeleteBuffers(1, &chunk.colour_mapping_buffer);
    }
    m_chunks.clear();

    std::size_t const lines = std::thread::hardware_concurrency();

    tf::Executor executor;
    tf::Taskflow taskflow;

    GLint data_size_location = glGetUniformLocation(m_shader_program, "u_data_size");

    if (m_mode.chunked) {
        glm::ivec3 const size{m_mode.chunk_size, m_mode.chunk_size, m_mode.chunk_size};
        glProgramUniform3iv(m_shader_program, data_size_location, 1, glm::value_ptr(size));
    } else {
        glProgramUniform3iv(m_shader_program, data_size_location, 1, glm::value_ptr(data.size()));
    }

    if (m_mode.chunked) {
        talloc::glNamedBufferData(m_depth_buffer, IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glClearNamedBufferData(m_depth_buffer, GL_R32F, 0, IMAGE_WIDTH * IMAGE_HEIGHT, nullptr);
    }

    GLint scale_location = glGetUniformLocation(m_shader_program, "u_scale");
    glProgramUniform1ui(m_shader_program, scale_location, SCALE);

    constexpr float vertices[] = {
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
    };

    talloc::glNamedBufferData(m_vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);

    std::shared_ptr<IVoxelDataProvider> provider;

    if (m_mode.chunked) {
        provider = std::make_shared<ChunkedDataProvider>(data, m_mode.chunk_size);
    } else {
        provider = std::make_shared<FullDataProvider>(data);
    }

    tracked_vector<IVoxelDataProvider::VoxelDataChunk> data_chunks;
    data_chunks.resize(lines);

    tracked_vector<Chunk> chunks;

    std::mutex chunks_mutex;
    std::condition_variable c;

    tf::Pipeline pipeline{
        lines,
        tf::Pipe{
            tf::PipeType::SERIAL,
            [&](tf::Pipeflow& pf) {
                auto v = provider->next();
                if (!v) {
                    pf.stop();
                    return;
                }

                data_chunks[pf.line()] = v.value();
            },
        },
        tf::Pipe{
            tf::PipeType::PARALLEL,
            [&](tf::Pipeflow& pf) {
                IVoxelDataProvider::VoxelDataChunk chunk = std::move(data_chunks[pf.line()]);
                data_chunks[pf.line()] = {};

                bool empty = true;

                tracked_vector<LayerDescriptor> layer_descriptors;
                tracked_vector<std::uint32_t> layers_data;
                tracked_vector<Layer> layers;
                layers.push_back({});
                layers.back().init(chunk.data.size());

                tracked_vector<VoxelData::Type> voxels;
                tracked_vector<VoxelData::Type> values;
                values.push_back(0);
                tracked_vector<glm::vec4> colours;
                colours.push_back(glm::vec4(0.0f));
                tracked_umap<VoxelData::Type, std::uint32_t> colour_map;

                for (std::size_t z = 0; z < chunk.data.size().z; z++) {
                    for (std::size_t y = 0; y < chunk.data.size().y; y++) {
                        for (std::size_t x = 0; x < chunk.data.size().x; x++) {
                            VoxelData::Type const voxel_value = chunk.data.get_voxel(glm::ivec3{x, y, z});
                            glm::ivec3 const position{x, y, z};

                            if (!mask->is_visible(position, voxel_value)) {
                                voxels.push_back(0);
                                continue;
                            }

                            std::uint32_t colour;

                            if (colour_map.contains(voxel_value)) {
                                colour = colour_map[voxel_value];
                            } else {
                                colour = colours.size();
                                colours.push_back(glm::vec4(transfer_function->colour(position, voxel_value), 1.0f));
                                colour_map.insert_or_assign(voxel_value, colour);
                                values.push_back(voxel_value);
                            }

                            empty = false;
                            voxels.push_back(colour);
                            layers.back().set(glm::ivec3{x, y, z});
                        }
                    }
                }

                while (!glm::any(glm::lessThanEqual(layers.back().size, glm::ivec3(1)))) {
                    layers.push_back(layers.back().next_layer());
                }
                layers.pop_back();

                while (!layers.empty()) {
                    Layer layer = std::move(layers.back());
                    layers.pop_back();

                    layer_descriptors.push_back(LayerDescriptor{
                        .size = layer.size,
                        .pointer = (std::uint32_t)layers_data.size(),
                    });

                    layers_data.insert(layers_data.end(), layer.data.begin(), layer.data.end());
                }

                layer_descriptors.push_back({
                    .size = chunk.data.size(),
                    .pointer = (std::uint32_t)-1,
                });

                if (!empty) {
                    std::lock_guard lock(chunks_mutex);
                    chunks.push_back(Chunk{
                        .layer_descriptors = std::move(layer_descriptors),
                        .layers_data = std::move(layers_data),
                        .voxels = std::move(voxels),
                        .colours = std::move(colours),
                        .values = values,
                        .model = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.position)),
                        .position = chunk.position,
                    });
                    c.notify_one();
                }
            },
        },
    };

    taskflow.composed_of(pipeline);
    auto future = executor.run(taskflow);

    bool producer_done = false;

    std::thread completion_wait([&] {
        future.wait();

        {
            std::lock_guard lock(chunks_mutex);
            producer_done = true;
        }

        c.notify_one();
    });

    while (true) {
        Chunk chunk;
        {
            std::unique_lock lock(chunks_mutex);
            c.wait(lock, [&] { return producer_done || !chunks.empty(); });

            if (chunks.empty() && producer_done) break;

            chunk = std::move(chunks.back());
            chunks.pop_back();
        }

        m_chunks.push_back({});

        glCreateBuffers(1, &m_chunks.back().layer_descriptors_buffer);
        glCreateBuffers(1, &m_chunks.back().layer_data_buffer);
        glCreateBuffers(1, &m_chunks.back().voxel_data_buffer);
        glCreateBuffers(1, &m_chunks.back().colour_mapping_buffer);

        talloc::glNamedBufferData(m_chunks.back().layer_descriptors_buffer,
                                  sizeof(LayerDescriptor) * chunk.layer_descriptors.size(),
                                  chunk.layer_descriptors.data(), GL_STATIC_READ);
        talloc::glNamedBufferData(m_chunks.back().layer_data_buffer, sizeof(std::uint32_t) * chunk.layers_data.size(),
                                  chunk.layers_data.data(), GL_STATIC_READ);
        talloc::glNamedBufferData(m_chunks.back().voxel_data_buffer, sizeof(VoxelData::Type) * chunk.voxels.size(),
                                  chunk.voxels.data(), GL_STATIC_READ);
        talloc::glNamedBufferData(m_chunks.back().colour_mapping_buffer, sizeof(glm::vec4) * chunk.colours.size(),
                                  chunk.colours.data(), GL_STATIC_READ);

        m_chunks.back().model = glm::translate(glm::mat4(1.0f), glm::vec3(chunk.position));
        m_chunks.back().position = chunk.position;
        m_chunks.back().layers = chunk.layer_descriptors.size();
        m_chunks.back().values = std::vector<VoxelData::Type>(chunk.values.begin(), chunk.values.end());
    }

    completion_wait.join();
}

void GridHierarchyRayMarchingRenderer::update_colour(std::shared_ptr<ITransferFunction> transfer_function) {
    for (auto& chunk : m_chunks) {
        std::vector<glm::vec4> new_colours;
        new_colours.reserve(chunk.values.size());

        for (auto value : chunk.values) {
            new_colours.push_back(glm::vec4(transfer_function->colour({0, 0, 0}, value), 1.0f));
        }

        glNamedBufferData(chunk.colour_mapping_buffer, sizeof(glm::vec4) * new_colours.size(), new_colours.data(),
                          GL_STATIC_READ);
    }
}

void GridHierarchyRayMarchingRenderer::clear() {
    if (m_mode.chunked) {
        glClear(GL_COLOR_BUFFER_BIT);
        float value = 64000.0f;
        glClearNamedBufferData(m_depth_buffer, GL_R32F, GL_RED, GL_FLOAT, &value);
    }
}

void GridHierarchyRayMarchingRenderer::render(Camera const& camera) {
    glm::dmat4 inv_vp = glm::inverse(camera.vp());

    if (m_mode.chunked && m_mode.sorted) {
        std::sort(m_chunks.begin(), m_chunks.end(), [&](UploadedChunk const& a, UploadedChunk const& b) {
            return glm::length(camera.position() - a.position) < glm::length(camera.position() - b.position);
        });
    }

    float pixel_delta_width = -1.0f - ((1.0f / IMAGE_WIDTH) * 2.0f - 1.0f);
    float pixel_delta_height = 1.0f - ((1.0f - (1.0f / IMAGE_HEIGHT)) * 2.0f - 1.0f);

    glm::dvec4 upper_left = inv_vp * glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    glm::dvec4 moved_width = inv_vp * glm::vec4(-1.0f - pixel_delta_width, -1.0f, 0.0f, 1.0f);
    glm::dvec4 moved_height = inv_vp * glm::vec4(-1.0f, -1.0f + pixel_delta_height, 0.0f, 1.0f);

    upper_left /= upper_left.w;
    moved_width /= moved_width.w;
    moved_height /= moved_height.w;

    for (auto& chunk : m_chunks) {
        glm::dmat4 inverse_model = glm::inverse(camera.model() * chunk.model);

        glm::vec4 upper_left_final = inverse_model * upper_left;
        glm::vec4 delta_width = inverse_model * (moved_width - upper_left);
        glm::vec4 delta_height = inverse_model * (moved_height - upper_left);
        glm::vec4 camera_position = inverse_model * glm::dvec4(camera.position(), 1.0f);

        GLint upper_left_location = glGetUniformLocation(m_shader_program, "u_upper_left");
        GLint delta_width_location = glGetUniformLocation(m_shader_program, "u_delta_width");
        GLint delta_height_location = glGetUniformLocation(m_shader_program, "u_delta_height");
        GLint camera_origin_location = glGetUniformLocation(m_shader_program, "u_camera_origin");
        GLint layers_location = glGetUniformLocation(m_shader_program, "u_layers");

        glProgramUniform3fv(m_shader_program, upper_left_location, 1, glm::value_ptr(upper_left_final));
        glProgramUniform3fv(m_shader_program, delta_width_location, 1, glm::value_ptr(delta_width));
        glProgramUniform3fv(m_shader_program, delta_height_location, 1, glm::value_ptr(delta_height));
        glProgramUniform3fv(m_shader_program, camera_origin_location, 1, glm::value_ptr(camera_position));
        glProgramUniform1ui(m_shader_program, layers_location, chunk.layers);

        glUseProgram(m_shader_program);
        glBindVertexArray(m_vao);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunk.layer_descriptors_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunk.layer_data_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, chunk.voxel_data_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, chunk.colour_mapping_buffer);

        if (m_mode.chunked) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_depth_buffer);
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

char const* GridHierarchyRayMarchingRenderer::name() {
    return "Grid hierarchy ray marching renderer";
}
