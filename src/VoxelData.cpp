#include "VoxelData.h"

#include <cstddef>
#include <format>
#include <limits>
#include <stdexcept>

#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/vector_relational.hpp"

VoxelData::VoxelData(glm::ivec3 const& size, bool full) {
    m_size = size;
    m_size_chunks = glm::ceil(glm::vec3(size) / float(CHUNK_SIZE));

    m_chunks.resize(m_size_chunks.x * m_size_chunks.y * m_size_chunks.z, {});
    if (full) {
        for (auto& chunk : m_chunks) {
            chunk.voxels.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, std::numeric_limits<Type>::min());
        }
    }
}

void VoxelData::set_voxel(glm::ivec3 position, Type value) {
    glm::ivec3 const chunk_position = position / CHUNK_SIZE;
    glm::ivec3 const voxel_position = position % CHUNK_SIZE;

    std::size_t const chunk_index =
        chunk_position.x + chunk_position.y * m_size_chunks.x + chunk_position.z * m_size_chunks.x * m_size_chunks.y;

    if (chunk_index > m_chunks.size()) {
        throw std::runtime_error(std::format("Trying to write past the chunk data ({} {} {}) at ({} {} {})", m_size.x,
                                             m_size.y, m_size.z, position.x, position.y, position.z));
    }

    Chunk& chunk = m_chunks[chunk_index];

    if (chunk.voxels.size() == 0) {
        chunk.voxels.resize(CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, std::numeric_limits<Type>::min());
    }

    std::size_t const voxel_index =
        voxel_position.x + voxel_position.y * CHUNK_SIZE + voxel_position.z * CHUNK_SIZE * CHUNK_SIZE;

    chunk.voxels[voxel_index] = value;
}

VoxelData::Type VoxelData::get_voxel(glm::ivec3 position) const {
    glm::ivec3 const chunk_position = position / CHUNK_SIZE;
    glm::ivec3 const voxel_position = position % CHUNK_SIZE;

    std::size_t const chunk_index =
        chunk_position.x + chunk_position.y * m_size_chunks.x + chunk_position.z * m_size_chunks.x * m_size_chunks.y;

    if (chunk_index >= m_chunks.size()) {
        throw std::runtime_error(std::format("Trying to read past the chunk data ({} {} {}) at ({} {} {})", m_size.x,
                                             m_size.y, m_size.z, position.x, position.y, position.z));
    }

    Chunk const& chunk = m_chunks.at(chunk_index);

    std::size_t const voxel_index =
        voxel_position.x + voxel_position.y * CHUNK_SIZE + voxel_position.z * CHUNK_SIZE * CHUNK_SIZE;

    if (chunk.voxels.size() == 0) {
        return std::numeric_limits<VoxelData::Type>::min();
    }

    return chunk.voxels.at(voxel_index);
}

VoxelData::View VoxelData::view(glm::ivec3 start, glm::ivec3 size) const {
    return VoxelData::View(*this, start, size);
}

glm::ivec3 VoxelData::size() const {
    return m_size;
}

VoxelData::View::View(VoxelData const& data, glm::ivec3 start, glm::ivec3 size) {
    p_data = &data;
    m_start = start;
    m_size = size;
}

VoxelData::Type VoxelData::View::get_voxel(glm::ivec3 position) const {
    if (glm::any(glm::greaterThanEqual(position, m_size))) {
        throw std::runtime_error("Trying to read past view");
    }

    if (glm::any(glm::greaterThanEqual(position + m_start, p_data->size()))) {
        return std::numeric_limits<VoxelData::Type>::min();
    }

    return p_data->get_voxel(position + m_start);
}

glm::ivec3 VoxelData::View::size() const {
    return m_size;
}
