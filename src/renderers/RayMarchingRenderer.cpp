#include "RayMarchingRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "config.h"
#include "data_providers/ChunkedDataProvider.h"
#include "data_providers/FullDataProvider.h"
#include "data_providers/IVoxelDataProvider.h"
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
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

RayMarchingRenderer::RayMarchingRenderer(Mode const& mode) {
    m_mode = mode;
}

void RayMarchingRenderer::init() {
    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source;
        if (m_mode.chunked) {
            fragment_source = utils::load_shader("assets/base_chunked_raymarch.frag");
        } else {
            fragment_source = utils::load_shader("assets/base_raymarch.frag");
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

void RayMarchingRenderer::deinit() {
    glDeleteProgram(m_shader_program);

    glDeleteBuffers(1, &m_vbo);

    for (auto const& chunk : m_chunks) {
        glDeleteBuffers(1, &chunk.colour_mapping_buffer);
        glDeleteBuffers(1, &chunk.voxel_data_buffer);
    }

    glDeleteVertexArrays(1, &m_vao);
}

void RayMarchingRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                      std::shared_ptr<IMask> mask) {
    GLint data_size_location = glGetUniformLocation(m_shader_program, "u_data_size");
    if (m_mode.chunked) {
        glm::ivec3 const size{m_mode.chunk_size, m_mode.chunk_size, m_mode.chunk_size};
        glProgramUniform3iv(m_shader_program, data_size_location, 1, glm::value_ptr(size));
    } else {
        glProgramUniform3iv(m_shader_program, data_size_location, 1, glm::value_ptr(data.size()));
    }

    talloc::glNamedBufferData(m_depth_buffer, IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glClearNamedBufferData(m_depth_buffer, GL_R32F, 0, IMAGE_WIDTH * IMAGE_HEIGHT, nullptr);

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

    while (auto maybe_chunk = provider->next()) {
        auto chunk = maybe_chunk.value();
        m_chunks.push_back({});

        glCreateBuffers(1, &m_chunks.back().voxel_data_buffer);
        glCreateBuffers(1, &m_chunks.back().colour_mapping_buffer);

        tracked_vector<VoxelData::Type> voxels;
        tracked_umap<VoxelData::Type, std::uint32_t> colour_map;
        tracked_vector<glm::vec4> colours;
        colours.push_back(glm::vec4(0.0f));

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
                    }

                    voxels.push_back(colour);
                }
            }
        }

        talloc::glNamedBufferData(m_chunks.back().colour_mapping_buffer, sizeof(glm::vec4) * colours.size(),
                                  colours.data(),

                                  GL_STATIC_READ);
        talloc::glNamedBufferData(m_chunks.back().voxel_data_buffer, sizeof(VoxelData::Type) * voxels.size(),
                                  voxels.data(), GL_STATIC_READ);

        glm::mat4 model{1.0f};
        m_chunks.back().model = glm::translate(model, glm::vec3(chunk.position));
        m_chunks.back().position = chunk.position;
    }
}

void RayMarchingRenderer::clear() {
    if (m_mode.chunked) {
        glClear(GL_COLOR_BUFFER_BIT);
        float value = 64000.0f;
        glClearNamedBufferData(m_depth_buffer, GL_R32F, GL_RED, GL_FLOAT, &value);
    }
}

void RayMarchingRenderer::render(Camera const& camera) {
    glm::dmat4 inv_vp = glm::inverse(camera.vp());

    if (m_mode.chunked && m_mode.sorted) {
        std::sort(m_chunks.begin(), m_chunks.end(), [&](Chunk const& a, Chunk const& b) {
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

    for (auto const& chunk : m_chunks) {
        glm::dmat4 inverse_model = glm::inverse(camera.model() * chunk.model);

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
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, chunk.voxel_data_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunk.colour_mapping_buffer);
        if (m_mode.chunked) {
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_depth_buffer);
        }

        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

char const* RayMarchingRenderer::name() {
    return "Ray marching renderer";
}
