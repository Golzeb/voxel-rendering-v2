#pragma once
#include <optional>

#include "VoxelData.h"
#include "data_providers/IVoxelDataProvider.h"

class FullDataProvider : public IVoxelDataProvider {
public:
    FullDataProvider(VoxelData const& data);
    ~FullDataProvider() override = default;

    std::optional<VoxelDataChunk> next() override;

private:
    bool m_read;
    VoxelData const* p_data;
};
