#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Camera.h"
#include "IRenderer.h"
#include "VoxelData.h"
#include "glad/gl.h"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class InstancedRasterRenderer : public IRenderer {
public:
    struct Mode {
        bool face_culled = false;
        bool indexed = false;
    };

private:
    struct InstanceDesc {
        GLuint vao;
        GLuint vbo;
        GLuint ibo;
        GLuint ebo;
        std::uint32_t vertex_count = 0;
        std::uint32_t instance_count = 0;
    };

public:
    InstancedRasterRenderer(Mode const& mode);
    ~InstancedRasterRenderer() override = default;

    void init() override;
    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;

    void update_colour(std::shared_ptr<ITransferFunction> transfer_function) override;

    void clear() override;
    void render(Camera const& camera) override;

    char const* name() override;

private:
    Mode m_mode;

    std::vector<VoxelData::Type> m_voxel_values;
    GLuint m_colour_buffer;

    std::vector<std::uint32_t> m_indices;

    std::vector<InstanceDesc> m_instances;

    GLuint m_shader_program;
};
