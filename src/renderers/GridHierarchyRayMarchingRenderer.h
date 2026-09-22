#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Camera.h"
#include "VoxelData.h"
#include "allocators/TrackingAllocator.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/ext/vector_int4.hpp"
#include "masks/IMask.h"
#include "renderers/IRenderer.h"
#include "transfer_functions/ITransferFunction.h"

class GridHierarchyRayMarchingRenderer : public IRenderer {
public:
    struct Mode {
        bool sorted = false;
        bool chunked = false;
        std::int32_t chunk_size = 0;
    };

public:
    GridHierarchyRayMarchingRenderer(Mode const& mode);

    void init() override;

    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;
    void clear() override;

    void render(Camera const& camera) override;

    char const* name() override;

private:
    struct LayerDescriptor {
        alignas(glm::ivec4) glm::ivec3 size;
        std::uint32_t pointer;
    };

    struct UploadedChunk {
        GLuint layer_descriptors_buffer;
        GLuint layer_data_buffer;
        GLuint voxel_data_buffer;
        GLuint colour_mapping_buffer;

        glm::mat4 model;
        glm::vec3 position;

        std::uint32_t layers;
    };

    struct Chunk {
        tracked_vector<LayerDescriptor> layer_descriptors;
        tracked_vector<std::uint32_t> layers_data;
        tracked_vector<VoxelData::Type> voxels;
        tracked_vector<glm::vec4> colours;

        glm::mat4 model;
        glm::vec3 position;

        std::uint32_t layers;
    };

    std::vector<UploadedChunk> m_chunks;

    GLuint m_shader_program;
    GLuint m_vbo;
    GLuint m_vao;
    GLuint m_depth_buffer;

    Mode m_mode;
};
