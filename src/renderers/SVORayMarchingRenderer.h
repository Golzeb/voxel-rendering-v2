#pragma once
#include <memory>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "glad/gl.h"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class SVORayMarchingRenderer : public IRenderer {
public:
    void init() override;

    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;

    void update_colour(std::shared_ptr<ITransferFunction> transfer_function) override;

    void clear() override;

    void render(Camera const& camera) override;

    char const* name() override;

private:
    std::vector<VoxelData::Type> m_values;

    GLuint m_shader_program;
    GLuint m_vbo;
    GLuint m_vao;

    GLuint m_colour_mapping_buffer;
    GLuint m_nodes_buffer;
    GLuint m_nodes_colour_buffer;
};
