#include "NoopRenderer.h"

#include <memory>

#include "Camera.h"
#include "VoxelData.h"
#include "glad/gl.h"
#include "masks/Mask.h"
#include "transfer_functions/ITransferFunction.h"

void NoopRenderer::init() {};
void NoopRenderer::deinit() {};

void NoopRenderer::update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                               Mask const& mask) {}

void NoopRenderer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
void NoopRenderer::render(Camera const& camera) {}

void NoopRenderer::update() {}

char const* NoopRenderer::name() {
    return "Noop Renderer";
}
