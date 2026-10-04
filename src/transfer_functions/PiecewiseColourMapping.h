#pragma once
#include <vector>

#include "ITransferFunction.h"
#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"

class PiecewiseColourMapping : public ITransferFunction {
private:
    struct Point {
        VoxelData::Type point;
        glm::vec3 colour;
    };

public:
    PiecewiseColourMapping() = default;
    ~PiecewiseColourMapping() = default;

    void add_point(glm::vec3 const& colour, VoxelData::Type point);

    glm::vec3 colour(glm::ivec3 voxel_position, VoxelData::Type voxel_value) override;
    bool controls() override;

private:
    std::vector<Point> m_points;
};
