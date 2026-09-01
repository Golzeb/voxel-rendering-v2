#pragma once

#include "IRenderer.h"

class NoopRenderer : public IRenderer {
public:
    ~NoopRenderer() override = default;

    void clear() override;
    void render() override;
    void update() override;
    char const* name() override;
};
