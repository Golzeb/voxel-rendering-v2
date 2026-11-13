#pragma once
#include <cstdint>
#include <mutex>
#include <optional>

#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/ext/vector_int3.hpp"

class ChunkedDataProvider : public IVoxelDataProvider {
public:
    ChunkedDataProvider(VoxelData const& data, std::int32_t chunk_size);
    ~ChunkedDataProvider() override = default;

    std::optional<VoxelDataChunk> next() override;

private:
    glm::ivec3 m_size;
    glm::ivec3 m_current_chunk;
    std::int32_t m_chunk_size;

    VoxelData const* p_data;
    std::mutex m_chunk_mutex;
};
