#pragma once
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

class SceneLoader {
public:
    enum class BultinScenes : std::int32_t {
        DEFAULT = 0,
        RANDOM,
        MENGER,
        SPHERE,
        CUBES,
        COUNT,
    };

    struct SceneTransform {
        glm::vec3 rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 origin = {0.0f, 0.0f, 0.0f};
        glm::vec3 scale = {1.0f, 1.0f, 1.0f};
        glm::vec3 position = {0.0f, 0.0f, 0.0f};
    };

    using get_t = std::tuple<VoxelData, std::shared_ptr<IMask>, std::shared_ptr<ITransferFunction>, SceneTransform>;

private:
    struct ColourMapValue {
        VoxelData::Type value;
        std::array<float, 3> colour = {0.0f, 0.0f, 0.0f};
    };

    struct ColourMap {
        std::string type;
        std::vector<ColourMapValue> values;
    };

    struct _SceneTransform {
        std::array<float, 3> rotation = {0.0f, 0.0f, 0.0f};
        std::array<float, 3> origin = {0.0f, 0.0f, 0.0f};
        std::array<float, 3> scale = {1.0f, 1.0f, 1.0f};
        std::array<float, 3> position = {0.0f, 0.0f, 0.0f};
    };

    struct Scene {
        std::string name;
        std::string file;
        std::string format;
        std::int32_t value_offset = 0;
        std::array<std::int32_t, 3> size;
        std::int32_t threshold;
        ColourMap colour_map;
        _SceneTransform transform;
    };

public:
    SceneLoader();
    ~SceneLoader() = default;

    get_t get(std::int32_t index, std::int32_t size = 0, bool noise = false);
    char const* const* names() const;
    std::int32_t count() const;

private:
    std::vector<Scene> m_scenes;
    std::vector<char const*> m_names;
};
