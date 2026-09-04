#include "NoopOptimizer.h"

#include <optional>

#include "Mesh.h"

std::optional<Mesh> NoopOptimizer::optimize_mesh(Mesh&& mesh) {
    return mesh;
}
