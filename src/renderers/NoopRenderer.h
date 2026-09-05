#pragma once
#include <memory>

#include "Camera.h"
#include "IRenderer.h"
#include "VoxelData.h"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class NoopRenderer : public IRenderer {
public:
    ~NoopRenderer() override = default;

    void init() override;
    void deinit() override;

    void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                     std::shared_ptr<IMask> mask) override;

    void clear() override;
    void render(Camera const& camera) override;

    char const* name() override;
};
