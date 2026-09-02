#pragma once
#include <optional>

#include "Mesh.h"
#include "optimizer/IMeshOptimizer.h"

class MeshIndexer : public IMeshOptimizer {
public:
    MeshIndexer() = default;

    std::optional<Mesh> optimize_mesh(Mesh&& mesh) override;
};
