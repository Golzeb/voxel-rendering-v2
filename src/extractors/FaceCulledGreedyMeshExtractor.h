#pragma once
#include <memory>
#include <optional>

#include "Mesh.h"
#include "data_providers/IVoxelDataProvider.h"
#include "extractors/IMeshExtractor.h"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class FaceCulledGreedyMeshExtractor : public IMeshExtractor {
public:
    FaceCulledGreedyMeshExtractor() = default;

    std::optional<Mesh> extract_mesh(IVoxelDataProvider::VoxelDataChunk const& chunk,
                                     std::shared_ptr<ITransferFunction> transfer_function,
                                     std::shared_ptr<IMask> mask) override;
};
