#include "ChunkedDataProvider.h"

#include <cstdint>
#include <mutex>
#include <optional>

#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

ChunkedDataProvider::ChunkedDataProvider(VoxelData const& data, std::int32_t chunk_size) {
    p_data = &data;
    m_chunk_size = chunk_size;
    m_current_chunk = glm::ivec3(0);

    m_size = glm::ceil(glm::vec3(data.size()) / float(chunk_size));
}

std::optional<IVoxelDataProvider::VoxelDataChunk> ChunkedDataProvider::next() {
    glm::ivec3 previous_position;
    {
        std::lock_guard lock(m_chunk_mutex);

        if (m_current_chunk.z >= m_size.z) {
            return std::nullopt;
        }

        previous_position = m_current_chunk;

        m_current_chunk.x += 1;

        m_current_chunk.y += m_current_chunk.x / m_size.x;
        m_current_chunk.x %= m_size.x;

        m_current_chunk.z += m_current_chunk.y / m_size.y;
        m_current_chunk.y %= m_size.y;
    }

    return IVoxelDataProvider::VoxelDataChunk{
        .position = previous_position * m_chunk_size,
        .data = p_data->view(previous_position * m_chunk_size, glm::ivec3(m_chunk_size)),
    };
}
