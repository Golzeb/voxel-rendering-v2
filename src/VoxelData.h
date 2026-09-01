#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

class VoxelData {
public:
    using Type = std::uint16_t;

private:
    static constexpr std::size_t CHUNK_SIZE = 16;
    struct Chunk {
        std::vector<std::uint16_t> voxels;
    };

public:
    VoxelData() = default;
    ~VoxelData() = default;

private:
    std::vector<Chunk> m_chunks;
};
