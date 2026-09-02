#pragma once
#include <optional>

#include "VoxelData.h"
#include "glm/ext/vector_int3.hpp"

class IVoxelDataProvider {
public:
    struct VoxelDataChunk {
        glm::ivec3 position;
        VoxelData data;
    };

public:
    virtual ~IVoxelDataProvider() = default;

    virtual std::optional<VoxelDataChunk> next() = 0;
    virtual void reset() = 0;
};
