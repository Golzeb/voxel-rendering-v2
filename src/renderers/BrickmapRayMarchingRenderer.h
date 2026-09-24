#pragma once
#include <memory>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "glad/gl.h"
#include "glm/ext/vector_int3.hpp"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class BrickmapRayMarchingRenderer : public IRenderer {
public:
    void init() override;

    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;
    void clear() override;

    void render(Camera const& camera) override;

    char const* name() override;

private:
    struct Chunk {
        tracked_vector<glm::ivec3> voxels;
        tracked_vector<VoxelData::Type> voxel_values;
        glm::ivec3 position;
    };

    GLuint m_shader_program;
    GLuint m_vbo;
    GLuint m_vao;

    GLuint m_brickmap_buffer;
    GLuint m_brickgrid_buffer;
    GLuint m_voxel_colour_bufffer;
    GLuint m_colour_mapping_buffer;
};
