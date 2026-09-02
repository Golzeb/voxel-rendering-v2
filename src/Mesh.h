#pragma once
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

    glNamedBufferData(umesh.vbo, sizeof(Vertex) * mesh.vertices.size(), mesh.vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(umesh.colour_buffer, sizeof(glm::vec4) * mesh.colours.size(), mesh.colours.data(),
                      GL_STATIC_DRAW);

    umesh.count = mesh.vertices.size();

    if (mesh.indices.size() > 0) {
        glCreateBuffers(1, &umesh.ebo);
        glVertexArrayElementBuffer(umesh.vao, umesh.ebo);
        glNamedBufferData(umesh.ebo, sizeof(std::uint32_t) * mesh.indices.size(), mesh.indices.data(), GL_STATIC_DRAW);
        umesh.count = mesh.indices.size();
    }

    return umesh;
}

inline void destroy_mesh(UploadedMesh mesh) {
    glDeleteVertexArrays(1, &mesh.vao);
    glDeleteBuffers(1, &mesh.vbo);
    glDeleteBuffers(1, &mesh.colour_buffer);

    if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
}
}  // namespace mesh
