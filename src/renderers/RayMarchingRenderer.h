#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class RayMarchingRenderer : public IRenderer {
public:
    struct Mode {
        bool chunked = false;
        std::int32_t chunk_size = 0;
    };

public:
    RayMarchingRenderer(Mode const& mode);

    void init() override;

    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;
    void clear() override;

    void render(Camera const& camera) override;

    char const* name() override;

private:
    struct Chunk {
        GLuint voxel_data_buffer;
        GLuint colour_mapping_buffer;
        glm::mat4 model;
    };

    std::vector<Chunk> m_chunks;

    GLuint m_shader_program;
    GLuint m_vbo;
    GLuint m_vao;
    GLuint m_depth_buffer;

    Mode m_mode;
};
