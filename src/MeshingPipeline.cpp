#include "MeshingPipeline.h"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "Mesh.h"
#include "allocators/TrackingAllocator.h"
#include "data_providers/IVoxelDataProvider.h"
#include "extractors/IMeshExtractor.h"
#include "masks/IMask.h"
#include "optimizers/IMeshOptimizer.h"
#include "taskflow/algorithm/pipeline.hpp"
#include "taskflow/core/executor.hpp"
#include "taskflow/core/taskflow.hpp"
#include "taskflow/taskflow.hpp"
#include "transfer_functions/ITransferFunction.h"

MeshingPipeline::MeshingPipeline(IVoxelDataProvider& provider, IMeshExtractor& extractor, IMeshOptimizer& optimizer) {
    p_provider = &provider;
    p_extractor = &extractor;
    p_optimizer = &optimizer;
}

void MeshingPipeline::run(std::function<void(Mesh&&)> finalize_callback,
                          std::shared_ptr<ITransferFunction> transfer_function, std::shared_ptr<IMask> mask,
                          std::size_t num_lines) {
    tf::Executor executor;
    tf::Taskflow taskflow;

    tracked_vector<IVoxelDataProvider::VoxelDataChunk> data_chunks;
    data_chunks.resize(num_lines);

    tracked_vector<Mesh> extractor_output;
    extractor_output.resize(num_lines);

    tracked_vector<Mesh> meshes;
    std::mutex meshes_mutex;
    std::condition_variable c;

    tf::Pipeline pipeline{
        num_lines,
        tf::Pipe{
            tf::PipeType::SERIAL,
            [&](tf::Pipeflow& pf) {
                auto v = p_provider->next();
                if (!v) {
                    pf.stop();
                    return;
                }

                data_chunks[pf.line()] = v.value();
            },
        },
        tf::Pipe{
            tf::PipeType::PARALLEL,
            [&](tf::Pipeflow& pf) {
                auto m = p_extractor->extract_mesh(data_chunks[pf.line()], transfer_function, mask);
                data_chunks[pf.line()] = {};
                if (m) {
                    extractor_output[pf.line()] = std::move(m.value());
                }
            },
        },
        tf::Pipe{
            tf::PipeType::PARALLEL,
            [&](tf::Pipeflow& pf) {
                auto m = p_optimizer->optimize_mesh(std::move(extractor_output[pf.line()]));
                extractor_output[pf.line()] = {};

                if (m) {
                    std::lock_guard lock(meshes_mutex);
                    meshes.push_back(std::move(m.value()));
                    c.notify_one();
                }
            },
        },
    };

    taskflow.composed_of(pipeline);
    auto future = executor.run(taskflow);

    bool producer_done = false;

    std::thread completion_wait([&] {
        future.wait();

        {
            std::lock_guard lock(meshes_mutex);
            producer_done = true;
        }

        c.notify_one();
    });

    while (true) {
        Mesh mesh;
        {
            std::unique_lock lock(meshes_mutex);
            c.wait(lock, [&] { return producer_done || !meshes.empty(); });

            if (meshes.empty() && producer_done) break;

            mesh = std::move(meshes.back());
            meshes.pop_back();
        }

        finalize_callback(std::move(mesh));
    }

    completion_wait.join();
}
