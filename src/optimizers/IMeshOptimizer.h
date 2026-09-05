#pragma once

#include <optional>

#include "Mesh.h"
class IMeshOptimizer {
public:
    virtual ~IMeshOptimizer() = default;

    virtual std::optional<Mesh> optimize_mesh(Mesh&& mesh) = 0;
};
