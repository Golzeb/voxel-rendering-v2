#pragma once

class IRenderer {
public:
    virtual ~IRenderer() = default;

    virtual void clear() = 0;
    virtual void render() = 0;
    virtual void update() = 0;
    virtual char const* name() = 0;
};
