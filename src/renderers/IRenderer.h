#pragma once
#include <memory>

#include "Camera.h"
#include "VoxelData.h"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void init() = 0;
    virtual void deinit() = 0;

    virtual void update_data(VoxelData& data, std::shared_ptr<ITransferFunction> transfer_function,
                             std::shared_ptr<IMask> mask) = 0;

    virtual void update_colour(std::shared_ptr<ITransferFunction> transfer_function) = 0;

    virtual void clear() = 0;
    virtual void render(Camera const& camera) = 0;

    virtual char const* name() = 0;
};
