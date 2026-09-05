#include "NoopMeshOptimizer.h"

#include <optional>

#include "Mesh.h"

std::optional<Mesh> NoopMeshOptimizer::optimize_mesh(Mesh&& mesh) {
    return mesh;
}
