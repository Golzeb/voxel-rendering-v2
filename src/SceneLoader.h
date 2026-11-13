#pragma once
#include <array>
#include <cstdint>
#include <glaze/core/context.hpp>
#include <glaze/core/istream_buffer.hpp>
#include <glaze/forward.hpp>
#include <glaze/json/read.hpp>
#include <glaze/json/write.hpp>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "Benchmark.h"
#include "VoxelData.h"
#include "glm/ext/vector_float3.hpp"
#include "masks/IMask.h"
#include "transfer_functions/ITransferFunction.h"

namespace glz {
template <>
struct from<JSON, glm::vec3> {
    template <auto Opts>
    static void op(glm::vec3& vec, is_context auto&& ctx, auto&& it, auto&& end) {
        std::array<float, 3> vec_vals;
        parse<JSON>::op<Opts>(vec_vals, ctx, it, end);
        vec.x = vec_vals[0];
        vec.y = vec_vals[1];
        vec.z = vec_vals[2];
    }
};

template <>
struct to<JSON, glm::vec3> {
    template <auto Opts>
    static void op(glm::vec3 const& vec, is_context auto&& ctx, auto&& b, auto&& ix) {
        std::array<float, 3> vec_vals{vec.x, vec.y, vec.z};
        serialize<JSON>::op<Opts>(vec_vals, ctx, b, ix);
    }
};
}  // namespace glz

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

    struct BenchmarkData {
        std::vector<Benchmark::Point> points;
        float scale;
    };

    using get_t = std::tuple<VoxelData, std::shared_ptr<IMask>, std::shared_ptr<ITransferFunction>, SceneTransform,
                             BenchmarkData>;

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
        float path_scale = 1.0f;
        std::string path_file;
    };

public:
    SceneLoader();
    ~SceneLoader() = default;

    get_t get(std::int32_t index, std::int32_t size = 0, bool noise = false);
    std::int32_t get_id_by_name(std::string const& name) const;
    char const* const* names() const;
    std::int32_t count() const;

private:
    std::vector<Scene> m_scenes;
    std::vector<char const*> m_names;
};
