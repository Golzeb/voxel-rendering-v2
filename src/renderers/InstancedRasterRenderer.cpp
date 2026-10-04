#include "InstancedRasterRenderer.h"

#include <GL/gl.h>

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "data_providers/FullDataProvider.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

struct InstanceVertex {
    glm::vec3 position;

    bool operator==(InstanceVertex const& other) const {
        return position == other.position;
    }
};

template <>
struct std::hash<InstanceVertex> {
    std::size_t operator()(InstanceVertex const& v) const {
        std::uint32_t x = std::bit_cast<std::uint32_t>(v.position.x);
        std::uint32_t y = std::bit_cast<std::uint32_t>(v.position.y);
        std::uint32_t z = std::bit_cast<std::uint32_t>(v.position.z);

        x = x * 1664525 + 1013904223;
        y = y * 1664525 + 1013904223;
        z = z * 1664525 + 1013904223;

        x += y * z;
        y += z * x;
        z += x * y;

        x ^= x >> 16;
        y ^= y >> 16;
        z ^= z >> 16;

        x += y * z;
        y += z * x;
        z += x * y;

        return (std::size_t(x ^ y ^ z) << 32);
    }
};

struct InstanceData {
    glm::vec3 position;
    std::uint32_t colour;
};

inline void add_quad(tracked_vector<InstanceVertex>& vertices, std::array<glm::vec3, 4> corners) {
    vertices.push_back({corners[2]});
    vertices.push_back({corners[1]});
    vertices.push_back({corners[0]});
    vertices.push_back({corners[0]});
    vertices.push_back({corners[3]});
    vertices.push_back({corners[2]});
}

inline void add_cube(tracked_vector<InstanceVertex>& vertices, glm::vec3 position, std::uint8_t neighbours) {
    glm::vec3 const position000 = position;
    glm::vec3 const position001 = position + glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 const position010 = position + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 const position011 = position + glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 const position100 = position + glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 const position101 = position + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 const position110 = position + glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 const position111 = position + glm::vec3(1.0f, 1.0f, 1.0f);

    if ((neighbours & 0b000001)) add_quad(vertices, {position001, position000, position010, position011});
    if ((neighbours & 0b000010)) add_quad(vertices, {position100, position101, position111, position110});
    if ((neighbours & 0b000100)) add_quad(vertices, {position100, position000, position001, position101});
    if ((neighbours & 0b001000)) add_quad(vertices, {position010, position110, position111, position011});
    if ((neighbours & 0b010000)) add_quad(vertices, {position000, position100, position110, position010});
    if ((neighbours & 0b100000)) add_quad(vertices, {position101, position001, position011, position111});
}

InstancedRasterRenderer::InstancedRasterRenderer(Mode const& mode) {
    m_mode = mode;
}

void InstancedRasterRenderer::init() {
    glEnable(GL_DEPTH_TEST);

    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source = utils::load_shader("assets/instanced_raster.frag");
        std::string vertex_source = utils::load_shader("assets/instanced_raster.vert");

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
}

void InstancedRasterRenderer::deinit() {
    glDeleteProgram(m_shader_program);

    glDeleteBuffers(1, &m_colour_buffer);
}

void InstancedRasterRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                          std::shared_ptr<IMask> mask) {
    std::shared_ptr<IVoxelDataProvider> provider = std::make_shared<FullDataProvider>(data);

    tracked_umap<VoxelData::Type, std::uint32_t> colour_map;
    tracked_vector<tracked_vector<InstanceData>> instance_data;
    tracked_vector<glm::vec4> colours;
    tracked_vector<VoxelData::Type> values;

    tracked_vector<tracked_vector<InstanceVertex>> vertices;
    tracked_vector<tracked_vector<std::uint32_t>> indices;

    if (m_mode.face_culled) {
        for (std::size_t i = 0; i < 63; i++) {
            vertices.push_back({});
            add_cube(vertices.back(), glm::vec3{0.0f, 0.0f, 0.0f}, i + 1);
        }
        instance_data.resize(63);
        m_instances.resize(63);
    } else {
        vertices.push_back({});
        add_cube(vertices.back(), glm::vec3{0.0f, 0.0f, 0.0f}, 0b111111);
        instance_data.resize(1);
        m_instances.resize(1);
    }

    if (m_mode.indexed) {
        for (std::size_t i = 0; i < vertices.size(); i++) {
            tracked_umap<InstanceVertex, std::uint32_t> vertex_index;
            tracked_vector<InstanceVertex> new_vertices;
            indices.push_back({});

            for (auto const& vertex : vertices[i]) {
                if (vertex_index.contains(vertex)) {
                    indices.back().push_back(vertex_index[vertex]);
                } else {
                    indices.back().push_back(new_vertices.size());
                    vertex_index.insert_or_assign(vertex, new_vertices.size());
                    new_vertices.push_back(vertex);
                }
            }
            vertices[i] = new_vertices;
        }
    }

    std::size_t total_instances = 0;

    while (auto chunk = provider->next()) {
        for (std::size_t x = 0; x < chunk->data.size().x; x++) {
            for (std::size_t y = 0; y < chunk->data.size().y; y++) {
                for (std::size_t z = 0; z < chunk->data.size().z; z++) {
                    VoxelData::Type const voxel_value = chunk->data.get_voxel(glm::ivec3{x, y, z});
                    glm::ivec3 const position{x, y, z};

                    if (!mask->is_visible(position, voxel_value)) continue;

                    std::uint32_t colour;
                    if (colour_map.contains(voxel_value)) {
                        colour = colour_map[voxel_value];
                    } else {
                        colour = colours.size();
                        colours.push_back(glm::vec4(transfer_function->colour(position, voxel_value), 1.0f));
                        values.push_back(voxel_value);
                        colour_map.insert_or_assign(voxel_value, colour);
                    }

                    if (m_mode.face_culled) {
                        std::uint8_t nmask = 0;
                        nmask |= !(x != 0 &&
                                   mask->is_visible(glm::vec3{x - 1, y, z}, data.get_voxel(glm::ivec3{x - 1, y, z})));
                        nmask |= !(x < data.size().x - 1 &&
                                   mask->is_visible(glm::vec3{x + 1, y, z}, data.get_voxel(glm::ivec3{x + 1, y, z})))
                                 << 1;

                        nmask |= !(y != 0 &&
                                   mask->is_visible(glm::vec3{x, y - 1, z}, data.get_voxel(glm::ivec3{x, y - 1, z})))
                                 << 2;
                        nmask |= !(y < data.size().y - 1 &&
                                   mask->is_visible(glm::vec3{x, y + 1, z}, data.get_voxel(glm::ivec3{x, y + 1, z})))
                                 << 3;

                        nmask |= !(z != 0 &&
                                   mask->is_visible(glm::vec3{x, y, z - 1}, data.get_voxel(glm::ivec3{x, y, z - 1})))
                                 << 4;
                        nmask |= !(z < data.size().y - 1 &&
                                   mask->is_visible(glm::vec3{x, y, z + 1}, data.get_voxel(glm::ivec3{x, y, z + 1})))
                                 << 5;

                        if (nmask != 0) {
                            instance_data[nmask - 1].push_back(InstanceData{
                                .position = position,
                                .colour = colour,
                            });
                            total_instances += 1;
                        }
                    } else {
                        instance_data.back().push_back(InstanceData{
                            .position = position,
                            .colour = colour,
                        });
                        total_instances += 1;
                    }
                }
            }
        }
    }

    for (std::size_t i = 0; i < m_instances.size(); i++) {
        InstanceDesc& curr = m_instances[i];

        if (m_mode.indexed) {
            curr.vertex_count = indices[i].size();
        } else {
            curr.vertex_count = vertices[i].size();
        }

        glCreateBuffers(1, &curr.ibo);

        talloc::glNamedBufferData(curr.ibo, instance_data[i].size() * sizeof(InstanceData), instance_data[i].data(),
                                  GL_STATIC_DRAW);

        curr.instance_count = instance_data[i].size();

        glCreateBuffers(1, &curr.vbo);
        talloc::glNamedBufferData(curr.vbo, sizeof(InstanceVertex) * vertices[i].size(), vertices[i].data(),
                                  GL_STATIC_DRAW);

        glCreateVertexArrays(1, &curr.vao);
        glVertexArrayVertexBuffer(curr.vao, 0, curr.vbo, 0, sizeof(InstanceVertex));
        glVertexArrayVertexBuffer(curr.vao, 1, curr.ibo, 0, sizeof(InstanceData));

        glVertexArrayAttribFormat(curr.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(InstanceVertex, position));
        glVertexArrayAttribBinding(curr.vao, 0, 0);
        glEnableVertexArrayAttrib(curr.vao, 0);

        glVertexArrayAttribFormat(curr.vao, 1, 3, GL_FLOAT, GL_FALSE, offsetof(InstanceData, position));
        glVertexArrayAttribBinding(curr.vao, 1, 1);
        glEnableVertexArrayAttrib(curr.vao, 1);

        glVertexArrayAttribIFormat(curr.vao, 2, 1, GL_UNSIGNED_INT, offsetof(InstanceData, colour));
        glVertexArrayAttribBinding(curr.vao, 2, 1);
        glEnableVertexArrayAttrib(curr.vao, 2);

        glVertexArrayBindingDivisor(curr.vao, 1, 1);

        if (m_mode.indexed) {
            glCreateBuffers(1, &curr.ebo);
            talloc::glNamedBufferData(curr.ebo, sizeof(std::uint32_t) * indices[i].size(), indices[i].data(),
                                      GL_STATIC_DRAW);
            glVertexArrayElementBuffer(curr.vao, curr.ebo);
        }
    }

    m_voxel_values = std::vector<VoxelData::Type>(values.begin(), values.end());

    glCreateBuffers(1, &m_colour_buffer);
    talloc::glNamedBufferData(m_colour_buffer, sizeof(glm::vec4) * colours.size(), colours.data(), GL_STATIC_DRAW);
}

void InstancedRasterRenderer::update_colour(std::shared_ptr<ITransferFunction> transfer_function) {
    std::vector<glm::vec4> new_colours;
    new_colours.reserve(m_voxel_values.size());

    for (auto value : m_voxel_values) {
        new_colours.push_back(glm::vec4(transfer_function->colour({0, 0, 0}, value), 1.0f));
    }

    glNamedBufferData(m_colour_buffer, sizeof(glm::vec4) * new_colours.size(), new_colours.data(), GL_STATIC_READ);
}

void InstancedRasterRenderer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void InstancedRasterRenderer::render(Camera const& camera) {
    glUseProgram(m_shader_program);

    GLint vp_location = glGetUniformLocation(m_shader_program, "u_mvp");
    GLint model_location = glGetUniformLocation(m_shader_program, "u_chunk");

    glm::mat4 chunk_transform = glm::mat4(1.0f);
    glm::mat4 mvp = camera.vp() * camera.model();

    for (auto const& desc : m_instances) {
        glProgramUniformMatrix4fv(m_shader_program, vp_location, 1, GL_FALSE, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(m_shader_program, model_location, 1, GL_FALSE, glm::value_ptr(chunk_transform));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_colour_buffer);

        glBindVertexArray(desc.vao);

        if (m_mode.indexed) {
            glDrawElementsInstanced(GL_TRIANGLES, desc.vertex_count, GL_UNSIGNED_INT, nullptr, desc.instance_count);
        } else {
            glDrawArraysInstanced(GL_TRIANGLES, 0, desc.vertex_count, desc.instance_count);
        }
    }
}

char const* InstancedRasterRenderer::name() {
    return "Instanced raster renderer";
}
