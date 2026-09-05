#pragma once
#include <optional>

#include "Mesh.h"
#include "optimizers/IMeshOptimizer.h"

class NoopMeshOptimizer : public IMeshOptimizer {
public:
    NoopMeshOptimizer() = default;

    std::optional<Mesh> optimize_mesh(Mesh&& mesh) override;
};
