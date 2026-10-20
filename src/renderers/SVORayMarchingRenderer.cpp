#include "SVORayMarchingRenderer.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "config.h"
#include "glm/ext/matrix_double4x4.hpp"
#include "glm/ext/vector_double4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"
#include "glm/vector_relational.hpp"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

std::size_t next_pow2(std::size_t value) {
    value -= 1;

    std::size_t shift = 1;
    while (shift < (sizeof(std::size_t) * 8)) {
        value |= value >> shift;
        shift *= 2;
    }

    return value + 1;
}

struct SparseVoxelOctree {
    struct Node {
        std::uint32_t mask;              // [ 16             | 8             | 8         ]
                                         // [ children count | children_mask | leaf_mask ]
        std::uint32_t children_pointer;  // Either colour or something

        void set_child(std::size_t index) {
            mask |= 0x1 << (index + 8);
        }

        void set_leaf(std::size_t index) {
            mask |= 0x1 << index;
        }

        void set_children_count(std::uint32_t count) {
            mask |= count << 16;
        }

        std::uint32_t get_children_count() {
            return (mask >> 16) & 0xFFFF;
        }
    };

    tracked_deque<tracked_vector<Node>> nodes;
    tracked_vector<Node> flattened_nodes;
    tracked_vector<std::uint32_t> colour_indices;
    std::uint32_t size;

    tracked_vector<glm::vec4> colours;
    tracked_vector<VoxelData::Type> colour_values;
    tracked_umap<VoxelData::Type, std::uint32_t> colour_map;

    bool rec(VoxelData const& data, std::shared_ptr<ITransferFunction> transfer_function, std::shared_ptr<IMask> mask,
             glm::ivec3 from, std::int32_t size, std::int32_t depth = 0) {
        constexpr VoxelData::Type min = std::numeric_limits<VoxelData::Type>::min();

        if (nodes.size() == depth) {
            nodes.push_back(tracked_vector<Node>{});
        }

        Node node{};

        std::int32_t const half_size = size / 2;
        std::uint32_t children_count = 0;

        if (size == 2) {
            bool empty = true;
            node.children_pointer = colour_indices.size();

            for (std::size_t i = 0; i < 8; i++) {
                std::int32_t const x_offset = ((i >> 2) & 0x1) * half_size;
                std::int32_t const y_offset = ((i >> 1) & 0x1) * half_size;
                std::int32_t const z_offset = ((i >> 0) & 0x1) * half_size;

                glm::ivec3 const voxel_position = from + glm::ivec3{x_offset, y_offset, z_offset};
                if (glm::any(glm::greaterThanEqual(voxel_position, data.size()))) {
                    continue;
                }

                VoxelData::Type voxel_value = data.get_voxel(voxel_position);
                if (!mask->is_visible(voxel_position, voxel_value)) {
                    continue;
                }

                children_count += 1;
                node.set_child(i);
                node.set_leaf(i);

                if (colour_map.contains(voxel_value)) {
                    colour_indices.push_back(colour_map[voxel_value]);
                } else {
                    colour_indices.push_back(colours.size());
                    colours.push_back(glm::vec4(transfer_function->colour(voxel_position, voxel_value), 1.0f));
                    colour_map.insert_or_assign(voxel_value, colour_indices.back());
                    colour_values.push_back(voxel_value);
                }

                empty = false;
            }

            if (!empty) {
                node.set_children_count(children_count);
                nodes[depth].push_back(node);
            }

            return !empty;
        }

        bool empty = true;
        for (std::size_t i = 0; i < 8; i++) {
            std::int32_t const x_offset = ((i >> 2) & 0x1) * half_size;
            std::int32_t const y_offset = ((i >> 1) & 0x1) * half_size;
            std::int32_t const z_offset = ((i >> 0) & 0x1) * half_size;

            if (rec(data, transfer_function, mask, from + glm::ivec3(x_offset, y_offset, z_offset), half_size,
                    depth + 1)) {
                node.set_child(i);
                empty = false;
                children_count += 1;
            }
        }

        if (!empty) {
            node.set_children_count(children_count);
            node.children_pointer = nodes[depth + 1].size() - children_count;
            nodes[depth].push_back(node);
        }

        return !empty;
    }

    void init(VoxelData const& data, std::shared_ptr<ITransferFunction> transfer_function,
              std::shared_ptr<IMask> mask) {
        size = next_pow2(std::max(std::max(data.size().x, data.size().y), data.size().z));

        rec(data, transfer_function, mask, glm::ivec3{0, 0, 0}, size);

        if (nodes[0].size() == 0) return;

        while (!nodes.empty()) {
            std::size_t const layer_size = nodes.front().size();
            std::size_t offset = flattened_nodes.size();

            for (std::size_t j = 0; j < layer_size; j++) {
                if ((nodes.front()[j].mask & 0xFF) == 0) nodes.front()[j].children_pointer += layer_size + offset;

                flattened_nodes.push_back(nodes.front()[j]);
            }

            nodes.pop_front();
        }
    }
};

void SVORayMarchingRenderer::init() {
    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source = utils::load_shader("assets/svo_raymarch.frag");
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

    glCreateBuffers(1, &m_colour_mapping_buffer);
    glCreateBuffers(1, &m_nodes_buffer);
    glCreateBuffers(1, &m_nodes_colour_buffer);
}

void SVORayMarchingRenderer::deinit() {
    glDeleteProgram(m_shader_program);

    glDeleteBuffers(1, &m_vbo);

    glDeleteBuffers(1, &m_colour_mapping_buffer);
    glDeleteBuffers(1, &m_nodes_buffer);
    glDeleteBuffers(1, &m_nodes_colour_buffer);

    glDeleteVertexArrays(1, &m_vao);
}

void SVORayMarchingRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                         std::shared_ptr<IMask> mask) {
    m_values.clear();

    constexpr float vertices[] = {
        -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f,
    };

    talloc::glNamedBufferData(m_vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);

    SparseVoxelOctree svo;
    svo.init(data, transfer_function, mask);

    m_values.insert(m_values.begin(), svo.colour_values.begin(), svo.colour_values.end());

    GLint data_size_location = glGetUniformLocation(m_shader_program, "u_data_size");
    glProgramUniform3i(m_shader_program, data_size_location, svo.size, svo.size, svo.size);

    talloc::glNamedBufferData(m_nodes_buffer, sizeof(SparseVoxelOctree::Node) * svo.flattened_nodes.size(),
                              svo.flattened_nodes.data(), GL_STATIC_READ);

    talloc::glNamedBufferData(m_nodes_colour_buffer, sizeof(std::uint32_t) * svo.colour_indices.size(),
                              svo.colour_indices.data(), GL_STATIC_READ);

    talloc::glNamedBufferData(m_colour_mapping_buffer, sizeof(glm::vec4) * svo.colours.size(), svo.colours.data(),
                              GL_STATIC_READ);
}

void SVORayMarchingRenderer::update_colour(std::shared_ptr<ITransferFunction> transfer_function) {
    std::vector<glm::vec4> new_colours;
    new_colours.reserve(m_values.size());

    for (auto value : m_values) {
        new_colours.push_back(glm::vec4(transfer_function->colour({0, 0, 0}, value), 1.0f));
    }

    glNamedBufferData(m_colour_mapping_buffer, sizeof(glm::vec4) * new_colours.size(), new_colours.data(),
                      GL_STATIC_READ);
}

void SVORayMarchingRenderer::clear() {}

void SVORayMarchingRenderer::render(Camera const& camera) {
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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_nodes_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodes_colour_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_colour_mapping_buffer);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

char const* SVORayMarchingRenderer::name() {
    return "SVO ray marching renderer";
}
