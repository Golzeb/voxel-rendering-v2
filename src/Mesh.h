#pragma once
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>

#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"

struct Vertex {
    glm::vec3 position;
    std::uint32_t colour;

    bool operator==(Vertex const& other) const {
        return position == other.position && colour == other.colour;
    }
};

template <>
struct std::hash<Vertex> {
    std::size_t operator()(Vertex const& v) const {
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

        return (std::size_t(x ^ y ^ z) << 32) | v.colour;
    }
};

struct Mesh {
    glm::mat4 model_matrix;
    tracked_vector<Vertex> vertices;
    tracked_vector<std::uint32_t> indices;
    tracked_vector<glm::vec4> colours;
    tracked_vector<VoxelData::Type> values;
};

struct UploadedMesh {
    glm::mat4 model_matrix;
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
    std::size_t count;

    GLuint colour_buffer;
    tracked_vector<VoxelData::Type> values;
};

namespace mesh {
inline UploadedMesh upload_mesh(Mesh&& mesh) {
    UploadedMesh umesh{};

    umesh.model_matrix = mesh.model_matrix;
    umesh.values = mesh.values;

    glCreateBuffers(1, &umesh.vbo);
    glCreateBuffers(1, &umesh.colour_buffer);
    glCreateVertexArrays(1, &umesh.vao);

    glVertexArrayVertexBuffer(umesh.vao, 0, umesh.vbo, 0, sizeof(Vertex));

    glVertexArrayAttribFormat(umesh.vao, 0, 3, GL_FLOAT, GL_FALSE, offsetof(Vertex, position));
    glEnableVertexArrayAttrib(umesh.vao, 0);

    glVertexArrayAttribIFormat(umesh.vao, 1, 1, GL_UNSIGNED_INT, offsetof(Vertex, colour));
    glEnableVertexArrayAttrib(umesh.vao, 1);

    glVertexArrayAttribBinding(umesh.vao, 0, 0);
    glVertexArrayAttribBinding(umesh.vao, 1, 0);

    talloc::glNamedBufferData(umesh.vbo, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);
    talloc::glNamedBufferData(umesh.colour_buffer, sizeof(glm::vec4) * mesh.colours.size(), mesh.colours.data(),
                              GL_STATIC_DRAW);

    umesh.count = mesh.vertices.size();

    if (mesh.indices.size() > 0) {
        glCreateBuffers(1, &umesh.ebo);
        glVertexArrayElementBuffer(umesh.vao, umesh.ebo);
        talloc::glNamedBufferData(umesh.ebo, sizeof(std::uint32_t) * mesh.indices.size(), mesh.indices.data(),
                                  GL_STATIC_DRAW);
        umesh.count = mesh.indices.size();
    }

    return umesh;
}

inline void destroy_mesh(UploadedMesh& mesh) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.colour_buffer);

    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
}

inline void add_quad(Mesh& mesh, std::uint32_t colour, std::array<glm::vec3, 4> corners) {
    mesh.vertices.push_back({corners[2], colour});
    mesh.vertices.push_back({corners[1], colour});
    mesh.vertices.push_back({corners[0], colour});
    mesh.vertices.push_back({corners[0], colour});
    mesh.vertices.push_back({corners[3], colour});
    mesh.vertices.push_back({corners[2], colour});
}

inline void add_cube(Mesh& mesh, std::uint32_t colour, glm::vec3 position,
                     std::array<bool, 6> const& neighbours = {false, false, false, false, false, false}) {
    glm::vec3 const position000 = position;
    glm::vec3 const position001 = position + glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 const position010 = position + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 const position011 = position + glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 const position100 = position + glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 const position101 = position + glm::vec3(1.0f, 0.0f, 1.0f);
    glm::vec3 const position110 = position + glm::vec3(1.0f, 1.0f, 0.0f);
    glm::vec3 const position111 = position + glm::vec3(1.0f, 1.0f, 1.0f);

    if (!neighbours[0]) add_quad(mesh, colour, {position001, position000, position010, position011});
    if (!neighbours[1]) add_quad(mesh, colour, {position100, position101, position111, position110});
    if (!neighbours[2]) add_quad(mesh, colour, {position100, position000, position001, position101});
    if (!neighbours[3]) add_quad(mesh, colour, {position010, position110, position111, position011});
    if (!neighbours[4]) add_quad(mesh, colour, {position000, position100, position110, position010});
    if (!neighbours[5]) add_quad(mesh, colour, {position101, position001, position011, position111});
}

}  // namespace mesh
