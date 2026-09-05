#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <thread>

#include "Mesh.h"
#include "data_providers/IVoxelDataProvider.h"
#include "extractors/IMeshExtractor.h"
#include "masks/IMask.h"
#include "optimizers/IMeshOptimizer.h"
#include "transfer_functions/ITransferFunction.h"
class MeshingPipeline {
public:
    MeshingPipeline(IVoxelDataProvider& provider, IMeshExtractor& extractor, IMeshOptimizer& optimizer);
    ~MeshingPipeline() = default;

    void run(std::function<void(Mesh&&)> finalize_callback, std::shared_ptr<ITransferFunction> transfer_function,
             std::shared_ptr<IMask> mask, std::size_t num_lines = std::thread::hardware_concurrency());

private:
    IVoxelDataProvider* p_provider;
    IMeshExtractor* p_extractor;
    IMeshOptimizer* p_optimizer;
};
