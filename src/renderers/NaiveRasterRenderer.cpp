#include "NaiveRasterRenderer.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Camera.h"
#include "Mesh.h"
#include "MeshingPipeline.h"
#include "VoxelData.h"
#include "data_providers/FullDataProvider.h"
#include "extractors/NaiveMeshExtractor.h"
#include "glad/gl.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "masks/Mask.h"
#include "optimizer/MeshIndexer.h"
#include "transfer_functions/ITransferFunction.h"
#include "utils.h"

void NaiveRasterRenderer::init() {
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

void NaiveRasterRenderer::deinit() {
    for (std::size_t i = 0; i < m_meshes.size(); i++) {
        mesh::destroy_mesh(m_meshes[i]);
    }

    glDeleteProgram(m_shader_program);

    glDisable(GL_DEPTH_TEST);
}

void NaiveRasterRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                                      Mask const& mask) {
    for (std::size_t i = 0; i < m_meshes.size(); i++) {
        mesh::destroy_mesh(m_meshes[i]);
    }

    FullDataProvider provider(data);
    NaiveMeshExtractor extractor;
    MeshIndexer indexer;

    MeshingPipeline pipeline(provider, extractor, indexer);

    auto callback = [this](Mesh&& mesh) {
        UploadedMesh umesh = mesh::upload_mesh(std::forward<Mesh>(mesh));
        m_meshes.push_back(umesh);
    };

    pipeline.run(callback, transfer_function, mask);
}

void NaiveRasterRenderer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void NaiveRasterRenderer::render(Camera const& camera) {
    glUseProgram(m_shader_program);

    GLint vp_location = glGetUniformLocation(m_shader_program, "u_vp");
    GLint model_location = glGetUniformLocation(m_shader_program, "u_model");

    for (auto const& mesh : m_meshes) {
        glm::mat4 vp = camera.vp() * camera.model();
        glProgramUniformMatrix4fv(m_shader_program, vp_location, 1, GL_FALSE, glm::value_ptr(vp));
        glProgramUniformMatrix4fv(m_shader_program, model_location, 1, GL_FALSE, glm::value_ptr(mesh.model_matrix));
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, mesh.colour_buffer);

        glBindVertexArray(mesh.vao);

        glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, nullptr);
    }
}

void NaiveRasterRenderer::update() {}

char const* NaiveRasterRenderer::name() {
    return "Raster renderer";
}
