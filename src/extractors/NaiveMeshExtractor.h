#pragma once
#include <memory>
#include <vector>

#include "Mesh.h"
#include "data_providers/IVoxelDataProvider.h"
#include "masks/Mask.h"
#include "transfer_functions/ITransferFunction.h"

class NaiveMeshExtractor {
public:
    NaiveMeshExtractor() = default;

    std::vector<Mesh> extract_meshes(IVoxelDataProvider& provider, std::shared_ptr<ITransferFunction> transfer_function,
                                     Mask const& mask);
};
