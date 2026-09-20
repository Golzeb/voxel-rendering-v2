#pragma once
#include <memory>

#include "Camera.h"
#include "VoxelData.h"
#include "glad/gl.h"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class RayMarchingRenderer : public IRenderer {
public:
    void init() override;

    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;
    void clear() override;

    void render(Camera const& camera) override;

    char const* name() override;

private:
    GLuint m_shader_program;
    GLuint m_vbo;
    GLuint m_vao;
    GLuint m_voxel_data_buffer;
    GLuint m_colour_mapping_buffer;
};
