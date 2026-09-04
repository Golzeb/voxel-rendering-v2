#pragma once
#include <memory>
#include <vector>

// clang-format off
#include "glad/gl.h"
// clang-format on 
#include "Camera.h"
#include "Mesh.h"
#include "VoxelData.h"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class IndexedRasterRenderer : public IRenderer {
public:
    void init() override;
    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function, IMask const& mask) override;

    void clear() override;
    void render(Camera const& camera) override;

    char const* name() override;

private:
    GLuint m_shader_program;
    std::vector<UploadedMesh> m_meshes;
};
