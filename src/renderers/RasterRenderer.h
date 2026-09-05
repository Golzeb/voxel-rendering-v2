#pragma once
#include <cstdint>
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

class RasterRenderer : public IRenderer {
public:
    struct Mode {
        bool indexed = false;
        bool face_culled = false;
        bool greedy_mesh = false;
        bool chunked = false;
        std::int32_t chunk_size = 0;
    };
public:
    RasterRenderer(Mode const& mode) ;
    
    void init() override;
    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function, IMask const& mask) override;

    void clear() override;
    void render(Camera const& camera) override;

    char const* name() override;

private:
    Mode m_mode;

    GLuint m_shader_program;
    std::vector<UploadedMesh> m_meshes;
};
