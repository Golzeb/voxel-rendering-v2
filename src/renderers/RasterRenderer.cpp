#include "RasterRenderer.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Camera.h"
#include "Mesh.h"
#include "MeshingPipeline.h"
#include "VoxelData.h"
#include "data_providers/ChunkedDataProvider.h"
#include "data_providers/FullDataProvider.h"
#include "data_providers/IVoxelDataProvider.h"
#include "extractors/FaceCulledGreedyMeshExtractor.h"
#include "extractors/FaceCulledMeshExtractor.h"
#include "extractors/IMeshExtractor.h"
#include "extractors/NaiveMeshExtractor.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/vector_float4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "masks/IMask.h"
#include "optimizers/IMeshOptimizer.h"
#include "optimizers/MeshIndexer.h"
#include "optimizers/NoopMeshOptimizer.h"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

RasterRenderer::RasterRenderer(Mode const& mode) {
    m_mode = mode;
}

void RasterRenderer::init() {
    glEnable(GL_DEPTH_TEST);

    m_shader_program = glCreateProgram();

    {
        GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

        std::string fragment_source = utils::load_shader("assets/base_raster.frag");
        std::string vertex_source = utils::load_shader("assets/base_raster.vert");

        char const* c_fragment_source = fragment_source.c_str();
        char const* c_vertex_source = vertex_source.c_str();

        glShaderSource(fragment_shader, 1, &c_fragment_source, nullptr);
        glShaderSource(vertex_shader, 1, &c_vertex_source, nullptr);

        glCompileShader(fragment_shader);
        utils::check_shader_compilation_errors(fragment_shader);

        glCompileShader(vertex_shader);
        utils::check_shader_compilation_errors(vertex_shader);

        glAttachShader(m_shader_program, fragment_shader);
        glAttachShader(m_shader_program, vertex_shader);

        glLinkProgram(m_shader_program);

        glDeleteShader(fragment_shader);
        glDeleteShader(vertex_shader);
    }
}

void RasterRenderer::deinit() {
    for (std::size_t i = 0; i < m_meshes.size(); i++) {
        mesh::destroy_mesh(m_meshes[i]);
    }

    glDeleteProgram(m_shader_program);

    glDisable(GL_DEPTH_TEST);
}

void RasterRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                 std::shared_ptr<IMask> mask) {
    for (std::size_t i = 0; i < m_meshes.size(); i++) {
        mesh::destroy_mesh(m_meshes[i]);
    }
    m_meshes.clear();

    std::unique_ptr<IVoxelDataProvider> provider;

    if (m_mode.chunked) {
        provider = std::make_unique<ChunkedDataProvider>(data, m_mode.chunk_size);
    } else {
        provider = std::make_unique<FullDataProvider>(data);
    }

    std::unique_ptr<IMeshExtractor> extractor;
    if (m_mode.greedy_mesh) {
        extractor = std::make_unique<FaceCulledGreedyMeshExtractor>();
    } else {
        if (m_mode.face_culled) {
            extractor = std::make_unique<FaceCulledMeshExtractor>();
        } else {
            extractor = std::make_unique<NaiveMeshExtractor>();
        }
    }

    std::unique_ptr<IMeshOptimizer> optimizer;

    if (m_mode.indexed) {
        optimizer = std::make_unique<MeshIndexer>();
    } else {
        optimizer = std::make_unique<NoopMeshOptimizer>();
    }

    MeshingPipeline pipeline(*provider, *extractor, *optimizer);

    auto callback = [this](Mesh&& mesh) {
        UploadedMesh umesh = mesh::upload_mesh(std::forward<Mesh>(mesh));
        m_meshes.push_back(umesh);
    };

    pipeline.run(callback, transfer_function, mask);
}

void RasterRenderer::update_colour(std::shared_ptr<ITransferFunction> transfer_function) {
    for (auto& mesh : m_meshes) {
        std::vector<glm::vec4> new_colours;
        new_colours.reserve(mesh.values.size());

        for (auto value : mesh.values) {
            new_colours.push_back(glm::vec4(transfer_function->colour({0, 0, 0}, value), 1.0f));
        }

        glNamedBufferData(mesh.colour_buffer, sizeof(glm::vec4) * new_colours.size(), new_colours.data(),
                          GL_STATIC_READ);
    }
}

void RasterRenderer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void RasterRenderer::render(Camera const& camera) {
    glUseProgram(m_shader_program);

    GLint vp_location = glGetUniformLocation(m_shader_program, "u_mvp");
    GLint model_location = glGetUniformLocation(m_shader_program, "u_chunk");

    for (auto const& mesh : m_meshes) {
        glm::mat4 mvp = camera.vp() * camera.model();
        glProgramUniformMatrix4fv(m_shader_program, vp_location, 1, GL_FALSE, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(m_shader_program, model_location, 1, GL_FALSE, glm::value_ptr(mesh.model_matrix));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.colour_buffer);

        glBindVertexArray(mesh.vao);

        if (m_mode.indexed) {
            glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, nullptr);
        } else {
            glDrawArrays(GL_TRIANGLES, 0, mesh.count);
        }
    }
}

char const* RasterRenderer::name() {
    return "Raster renderer";
}
