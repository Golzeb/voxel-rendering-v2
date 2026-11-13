#include "BrickmapRayMarchingRenderer.h"

#include <array>
#include <cmath>
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
#include "data_providers/IVoxelDataProvider.h"
#include "glm/common.hpp"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"
#include "masks/IMask.h"
#include "taskflow/algorithm/pipeline.hpp"
#include "taskflow/core/executor.hpp"
#include "taskflow/core/taskflow.hpp"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

constexpr std::int32_t BRICKMAP_SIZE = 8;
constexpr std::int32_t SUPERGRID_SIZE = 8;

struct Brickmap {
    std::array<std::uint32_t, BRICKMAP_SIZE * BRICKMAP_SIZE * BRICKMAP_SIZE / 32> data;
    std::uint32_t colour_pointer;

    void set(glm::ivec3 position) {
        std::size_t const index = position.x + position.y * BRICKMAP_SIZE + position.z * BRICKMAP_SIZE * BRICKMAP_SIZE;

        data[index >> 5] |= (1 << (index & 0x1F));
    }

    bool get(glm::ivec3 position) {
        std::size_t const index = position.x + position.y * BRICKMAP_SIZE + position.z * BRICKMAP_SIZE * BRICKMAP_SIZE;

        return (data[index >> 5] & (1 << (index & 0x1F)));
    }
};

struct BrickmapData {
    Brickmap brickmap;
    glm::ivec3 position;
    tracked_vector<VoxelData::Type> voxel_values;
};

void BrickmapRayMarchingRenderer::init() {
    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source = utils::load_shader("assets/brickmap_raymarch.frag");
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

    glCreateBuffers(1, &m_brickmap_buffer);
    glCreateBuffers(1, &m_brickgrid_buffer);
    glCreateBuffers(1, &m_supergrid_buffer);
    glCreateBuffers(1, &m_colour_mapping_buffer);
    glCreateBuffers(1, &m_voxel_colour_bufffer);
}

void BrickmapRayMarchingRenderer::deinit() {
    glDeleteProgram(m_shader_program);

    glDeleteBuffers(1, &m_vbo);

    glDeleteBuffers(1, &m_brickmap_buffer);
    glDeleteBuffers(1, &m_brickgrid_buffer);
    glDeleteBuffers(1, &m_supergrid_buffer);
    glDeleteBuffers(1, &m_colour_mapping_buffer);
    glDeleteBuffers(1, &m_voxel_colour_bufffer);

    glDeleteVertexArrays(1, &m_vao);
}

void BrickmapRayMarchingRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                              std::shared_ptr<IMask> mask) {
    m_values.clear();

    std::size_t const lines = std::thread::hardware_concurrency();

    tf::Executor executor;
    tf::Taskflow taskflow;

    GLint data_size_location = glGetUniformLocation(m_shader_program, "u_data_size");

    glm::ivec3 const data_size = glm::ceil(glm::vec3(data.size()) / float(BRICKMAP_SIZE));
    glProgramUniform3iv(m_shader_program, data_size_location, 1, glm::value_ptr(data_size));

    constexpr float vertices[] = {
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
    };

    talloc::glNamedBufferData(m_vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);

    std::shared_ptr<IVoxelDataProvider> provider = std::make_shared<ChunkedDataProvider>(data, BRICKMAP_SIZE);

    tracked_vector<IVoxelDataProvider::VoxelDataChunk> data_chunks;
    data_chunks.resize(lines);

    tracked_vector<BrickmapData> brickmap_data;

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
                BrickmapData data{};
                data.position = chunk.position / BRICKMAP_SIZE;

                for (std::size_t z = 0; z < chunk.data.size().z; z++) {
                    for (std::size_t y = 0; y < chunk.data.size().y; y++) {
                        for (std::size_t x = 0; x < chunk.data.size().x; x++) {
                            VoxelData::Type const voxel_value = chunk.data.get_voxel(glm::ivec3{x, y, z});

                            if (!mask->is_visible(glm::ivec3{x, y, z} + chunk.position, voxel_value)) {
                                continue;
                            }

                            data.brickmap.set(glm::ivec3{x, y, z});
                            data.voxel_values.push_back(voxel_value);
                            empty = false;
                        }
                    }
                }

                if (!empty) {
                    std::lock_guard lock(chunks_mutex);
                    brickmap_data.push_back(std::move(data));
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

    tracked_vector<Brickmap> brickmaps;
    tracked_vector<std::uint32_t> brickgrid;
    brickgrid.resize(data_size.x * data_size.y * data_size.z, 0);

    tracked_vector<glm::vec4> colours;
    tracked_umap<VoxelData::Type, std::uint32_t> colour_map;
    tracked_vector<std::uint32_t> brickmap_colours;

    glm::ivec3 supergrid_size = glm::ceil(glm::vec3(data_size) / float(SUPERGRID_SIZE));

    tracked_vector<std::uint32_t> supergrid;
    supergrid.resize(std::ceil(supergrid_size.x * supergrid_size.y * supergrid_size.z / 32.0f), 0);

    while (true) {
        BrickmapData brickmap;
        {
            std::unique_lock lock(chunks_mutex);
            c.wait(lock, [&] { return producer_done || !brickmap_data.empty(); });

            if (brickmap_data.empty() && producer_done) break;

            brickmap = std::move(brickmap_data.back());
            brickmap_data.pop_back();
        }

        brickmap.brickmap.colour_pointer = brickmap_colours.size();

        for (auto voxel_value : brickmap.voxel_values) {
            if (colour_map.contains(voxel_value)) {
                brickmap_colours.push_back(colour_map.at(voxel_value));
            } else {
                colour_map.insert({voxel_value, colours.size()});
                brickmap_colours.push_back(colours.size());
                colours.push_back(glm::vec4(transfer_function->colour(glm::ivec3(0), voxel_value), 1.0f));
                m_values.push_back(voxel_value);
            }
        }

        brickmaps.push_back(brickmap.brickmap);
        std::size_t const brickgrid_index =
            brickmap.position.x + brickmap.position.y * data_size.x + brickmap.position.z * data_size.x * data_size.y;

        glm::ivec3 const supergrid_position = brickmap.position / SUPERGRID_SIZE;
        std::size_t const supergrid_index = supergrid_position.x + supergrid_position.y * supergrid_size.x +
                                            supergrid_position.z * supergrid_size.x * supergrid_size.y;

        supergrid[supergrid_index / 32] |= 1 << (supergrid_index % 32);
        brickgrid[brickgrid_index] = brickmaps.size();
    }

    completion_wait.join();

    talloc::glNamedBufferData(m_supergrid_buffer, sizeof(std::uint32_t) * supergrid.size(), supergrid.data(),
                              GL_STATIC_READ);
    talloc::glNamedBufferData(m_brickgrid_buffer, sizeof(std::uint32_t) * brickgrid.size(), brickgrid.data(),
                              GL_STATIC_READ);
    talloc::glNamedBufferData(m_brickmap_buffer, sizeof(Brickmap) * brickmaps.size(), brickmaps.data(), GL_STATIC_READ);
    talloc::glNamedBufferData(m_voxel_colour_bufffer, sizeof(std::uint32_t) * brickmap_colours.size(),
                              brickmap_colours.data(), GL_STATIC_READ);
    talloc::glNamedBufferData(m_colour_mapping_buffer, sizeof(glm::vec4) * colours.size(), colours.data(),
                              GL_STATIC_READ);
}

void BrickmapRayMarchingRenderer::update_colour(std::shared_ptr<ITransferFunction> transfer_function) {
    std::vector<glm::vec4> new_colours;
    new_colours.reserve(m_values.size());

    for (auto value : m_values) {
        new_colours.push_back(glm::vec4(transfer_function->colour({0, 0, 0}, value), 1.0f));
    }

    glNamedBufferData(m_colour_mapping_buffer, sizeof(glm::vec4) * new_colours.size(), new_colours.data(),
                      GL_STATIC_READ);
}

void BrickmapRayMarchingRenderer::clear() {}

void BrickmapRayMarchingRenderer::render(Camera const& camera) {
    glm::dmat4 inv_vp = glm::inverse(camera.vp());

    float pixel_delta_width = -1.0f - ((1.0f / IMAGE_WIDTH) * 2.0f - 1.0f);
    float pixel_delta_height = 1.0f - ((1.0f - (1.0f / IMAGE_HEIGHT)) * 2.0f - 1.0f);

    glm::dvec4 upper_left = inv_vp * glm::vec4(-1.0f, -1.0f, 0.0f, 1.0f);
    glm::dvec4 moved_width = inv_vp * glm::vec4(-1.0f - pixel_delta_width, -1.0f, 0.0f, 1.0f);
    glm::dvec4 moved_height = inv_vp * glm::vec4(-1.0f, -1.0f + pixel_delta_height, 0.0f, 1.0f);

    upper_left /= upper_left.w;
    moved_width /= moved_width.w;
    moved_height /= moved_height.w;

    glm::dmat4 inverse_model = glm::inverse(camera.model());

    glm::vec4 upper_left_final = inverse_model * upper_left;
    glm::vec4 delta_width = inverse_model * (moved_width - upper_left);
    glm::vec4 delta_height = inverse_model * (moved_height - upper_left);
    glm::vec4 camera_position = inverse_model * glm::dvec4(camera.position(), 1.0f);

    GLint upper_left_location = glGetUniformLocation(m_shader_program, "u_upper_left");
    GLint delta_width_location = glGetUniformLocation(m_shader_program, "u_delta_width");
    GLint delta_height_location = glGetUniformLocation(m_shader_program, "u_delta_height");
    GLint camera_origin_location = glGetUniformLocation(m_shader_program, "u_camera_origin");

    glProgramUniform3fv(m_shader_program, upper_left_location, 1, glm::value_ptr(upper_left_final));
    glProgramUniform3fv(m_shader_program, delta_width_location, 1, glm::value_ptr(delta_width));
    glProgramUniform3fv(m_shader_program, delta_height_location, 1, glm::value_ptr(delta_height));
    glProgramUniform3fv(m_shader_program, camera_origin_location, 1, glm::value_ptr(camera_position));

    glUseProgram(m_shader_program);
    glBindVertexArray(m_vao);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_supergrid_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_brickgrid_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_brickmap_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_voxel_colour_bufffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, m_colour_mapping_buffer);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

char const* BrickmapRayMarchingRenderer::name() {
    return "Brickmap ray marching renderer";
}
