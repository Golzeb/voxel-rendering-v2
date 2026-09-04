#pragma once
#include <optional>

#include "Mesh.h"
#include "optimizer/IMeshOptimizer.h"

class NoopOptimizer : public IMeshOptimizer {
public:
    NoopOptimizer() = default;

    std::optional<Mesh> optimize_mesh(Mesh&& mesh) override;
};
