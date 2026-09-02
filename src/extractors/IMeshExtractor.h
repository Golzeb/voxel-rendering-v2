#pragma once
#include <memory>
#include <optional>

#include "Mesh.h"
#include "data_providers/IVoxelDataProvider.h"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class IMeshExtractor {
public:
    virtual ~IMeshExtractor() = default;
    virtual std::optional<Mesh> extract_mesh(IVoxelDataProvider::VoxelDataChunk const& chunk,
                                             std::shared_ptr<ITransferFunction> transfer_function,
                                             IMask const& mask) = 0;
};
