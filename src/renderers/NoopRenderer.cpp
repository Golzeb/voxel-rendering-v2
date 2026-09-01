#include "NoopRenderer.h"

#include "glad/gl.h"

void NoopRenderer::clear() {
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void NoopRenderer::render() {}

void NoopRenderer::update() {}

char const* NoopRenderer::name() {
    return "Noop Renderer";
}
