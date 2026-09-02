#pragma once
#include <cstdint>
#include <vector>

#include "glm/ext/vector_int3.hpp"

class VoxelData {
public:
    using Type = std::uint16_t;

private:
    static constexpr int CHUNK_SIZE = 16;
    struct Chunk {
        std::vector<std::uint16_t> voxels;
    };

public:
    VoxelData() = default;
    VoxelData(glm::ivec3 const& size, bool full = false);
    ~VoxelData() = default;

    void set_voxel(glm::ivec3 const& position, Type value);
    VoxelData::Type get_voxel(glm::ivec3 const& position) const;

    glm::ivec3 size() const;

private:
    std::vector<Chunk> m_chunks;

    glm::ivec3 m_size;
    glm::ivec3 m_size_chunks;
};
