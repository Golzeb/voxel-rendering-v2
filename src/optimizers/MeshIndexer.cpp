#include "MeshIndexer.h"

#include <cstdint>
#include <optional>

#include "Mesh.h"
#include "allocators/TrackingAllocator.h"

std::optional<Mesh> MeshIndexer::optimize_mesh(Mesh&& mesh) {
    tracked_umap<Vertex, std::uint32_t> vertex_indices;
    Mesh new_mesh;

    new_mesh.model_matrix = mesh.model_matrix;
    new_mesh.colours = mesh.colours;
    new_mesh.values = mesh.values;

    for (auto const& v : mesh.vertices) {
        if (vertex_indices.contains(v)) {
            new_mesh.indices.push_back(vertex_indices[v]);
        } else {
            new_mesh.indices.push_back(new_mesh.vertices.size());
            vertex_indices.insert_or_assign(v, new_mesh.vertices.size());

            new_mesh.vertices.push_back(v);
        }
    }

    return new_mesh;
}
