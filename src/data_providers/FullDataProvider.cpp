#include "FullDataProvider.h"

#include <optional>

#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"
#include "glm/ext/vector_int3.hpp"

FullDataProvider::FullDataProvider(VoxelData const& data) {
    p_data = &data;
    m_read = false;
}

std::optional<IVoxelDataProvider::VoxelDataChunk> FullDataProvider::next() {
    if (m_read) {
        return std::nullopt;
    }

    m_read = true;

    return IVoxelDataProvider::VoxelDataChunk{
        .position = glm::ivec3(0),
        .data = p_data->view(glm::ivec3(0), p_data->size()),
    };
}
