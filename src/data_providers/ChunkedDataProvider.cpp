#include "ChunkedDataProvider.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>

#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/vector_relational.hpp"
#include "taskflow/core/executor.hpp"
#include "taskflow/taskflow.hpp"

ChunkedDataProvider::ChunkedDataProvider(VoxelData const& data, std::int32_t chunk_size) {
    p_data = &data;
    m_chunk_size = chunk_size;
    m_current_chunk = glm::ivec3(0);

    m_size = glm::ceil(glm::vec3(data.size()) / float(chunk_size));
}

std::optional<IVoxelDataProvider::VoxelDataChunk> ChunkedDataProvider::next() {
    VoxelData chunk_data((glm::ivec3(m_chunk_size)), true);

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

    tf::Executor executor;

    for (std::size_t x = 0; x < m_chunk_size; x++) {
        executor.silent_async([this, &executor, x, previous_position, &chunk_data]() {
            for (std::size_t y = 0; y < m_chunk_size; y++) {
                executor.silent_async([this, x, y, previous_position, &chunk_data]() {
                    for (std::size_t z = 0; z < m_chunk_size; z++) {
                        glm::ivec3 const position = previous_position * m_chunk_size + glm::ivec3{x, y, z};
                        if (glm::any(glm::greaterThanEqual(position, p_data->size()))) continue;
                        chunk_data.set_voxel(glm::ivec3{x, y, z}, p_data->get_voxel(position));
                    }
                });
            }
        });
    }

    executor.wait_for_all();

    return IVoxelDataProvider::VoxelDataChunk{
        .position = previous_position * m_chunk_size,
        .data = chunk_data,
    };
}

void ChunkedDataProvider::reset() {
    m_current_chunk = glm::ivec3(0);
}
