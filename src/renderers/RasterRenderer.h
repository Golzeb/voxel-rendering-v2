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
        std::int32_t chunk_size = 64;
    };
public:
    RasterRenderer(Mode const& mode);
    ~RasterRenderer() override = default;
    
    void init() override;
    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function, std::shared_ptr<IMask> mask) override;

    void update_colour(std::shared_ptr<ITransferFunction> transfer_function) override;

    void clear() override;
    void render(Camera const& camera) override;

    char const* name() override;

private:
    Mode m_mode;

    GLuint m_shader_program;
    std::vector<UploadedMesh> m_meshes;
};
