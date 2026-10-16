#include "SceneLoader.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glaze/core/istream_buffer.hpp>
#include <glaze/json/read.hpp>
#include <limits>
#include <memory>
#include <random>
#include <utility>

#include "VoxelData.h"
#include "glm/common.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/ext/vector_int3.hpp"
#include "glm/geometric.hpp"
#include "loaders.h"
#include "masks/IMask.h"
#include "masks/ThresholdMask.h"
#include "spdlog/spdlog.h"
#include "transfer_functions/ITransferFunction.h"
#include "transfer_functions/PiecewiseColourMapping.h"
#include "transfer_functions/ValueColourMapping.h"

namespace builtin {
void add_noise(VoxelData& data, std::shared_ptr<IMask> mask, std::int32_t max, float chance, std::size_t seed = 0) {
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(0.0, 1.0);

    for (std::size_t z = 0; z < data.size().z; z++) {
        for (std::size_t y = 0; y < data.size().y; y++) {
            for (std::size_t x = 0; x < data.size().x; x++) {
                VoxelData::Type const voxel_value = data.get_voxel({x, y, z});
                if (!mask->is_visible(glm::ivec3{x, y, z}, voxel_value)) continue;

                if (dist(rng) < chance) {
                    std::int32_t const new_value = (std::int32_t)voxel_value + (rng() % 2 ? 1 : -1) * (rng() % max);
                    data.set_voxel({x, y, z},
                                   std::clamp<std::int32_t>(new_value, 0, std::numeric_limits<VoxelData::Type>::max()));
                }
            }
        }
    }
}

SceneLoader::get_t generate_default_scene() {
    auto mask = std::make_shared<ThresholdMask>(0, true);
    auto transfer_function = std::make_shared<ValueColourMapping>();

    transfer_function->set_colour(1, glm::vec3{1.0f, 1.0f, 1.0f});
    transfer_function->set_colour(2, glm::vec3(0.0f));

    VoxelData data(glm::ivec3{2, 2, 2});

    data.set_voxel(glm::ivec3{0, 0, 0}, 1);
    data.set_voxel(glm::ivec3{1, 1, 1}, 2);

    return {data, mask, transfer_function, SceneLoader::SceneTransform{.origin = {1, 1, 1}}};
}

SceneLoader::get_t generate_random(std::int32_t size, std::size_t seed = 0) {
    std::mt19937 rng(seed);

    std::uniform_real_distribution<float> dist(0.0, 1.0);

    auto mask = std::make_shared<ThresholdMask>(0, true);
    auto transfer_function = std::make_shared<PiecewiseColourMapping>();

    transfer_function->add_point(glm::vec3{0.1f, 0.1f, 0.1f}, 0);
    transfer_function->add_point(glm::vec3{0.9f, 0.9f, 0.9f}, 255);

    VoxelData data(glm::ivec3{size, size, size});

    for (std::size_t z = 0; z < data.size().z; z++) {
        for (std::size_t y = 0; y < data.size().y; y++) {
            for (std::size_t x = 0; x < data.size().x; x++) {
                if (dist(rng) >= 0.5) {
                    data.set_voxel(glm::ivec3{x, y, z}, rng() % 256);
                }
            }
        }
    }

    return {data, mask, transfer_function,
            SceneLoader::SceneTransform{.origin = {size / 2.0f, size / 2.0f, size / 2.0f}}};
}

SceneLoader::get_t generate_menger_sponge(std::int32_t size, VoxelData::Type empty_value, VoxelData::Type fill_value,
                                          bool noise) {
    auto mask = std::make_shared<ThresholdMask>(empty_value, true);
    auto transfer_function = std::make_shared<PiecewiseColourMapping>();

    transfer_function->add_point(glm::vec3{0.549f, 0.545f, 0.141f}, empty_value);
    transfer_function->add_point(glm::vec3{0.929f, 0.918f, 0.239f}, fill_value);

    VoxelData data(glm::ivec3{size, size, size});

    auto is_full = [](glm::ivec3 pos) -> bool {
        glm::ivec3 temp{};

        temp = pos;
        while (temp.x > 0 || temp.y > 0) {
            if (temp.x % 3 == 1 && temp.y % 3 == 1) return false;
            temp.x /= 3;
            temp.y /= 3;
        }

        temp = pos;
        while (temp.x > 0 || temp.z > 0) {
            if (temp.x % 3 == 1 && temp.z % 3 == 1) return false;
            temp.x /= 3;
            temp.z /= 3;
        }

        temp = pos;
        while (temp.z > 0 || temp.y > 0) {
            if (temp.z % 3 == 1 && temp.y % 3 == 1) return false;
            temp.z /= 3;
            temp.y /= 3;
        }

        return true;
    };

    for (std::size_t z = 0; z < data.size().z; z++) {
        for (std::size_t y = 0; y < data.size().y; y++) {
            for (std::size_t x = 0; x < data.size().x; x++) {
                if (is_full({x, y, z})) {
                    glm::vec3 dist = glm::abs(glm::ivec3{x, y, z} - glm::ivec3{size / 2, size / 2, size / 2});

                    float step_size = float(fill_value - empty_value) / (float(size) / 2);

                    data.set_voxel(glm::ivec3{x, y, z}, empty_value + std::max({dist.x, dist.y, dist.z}) * step_size);
                } else {
                    data.set_voxel(glm::ivec3{x, y, z}, empty_value);
                }
            }
        }
    }

    if (noise) add_noise(data, mask, (fill_value - empty_value) / 10, 0.2f);

    return {data, mask, transfer_function,
            SceneLoader::SceneTransform{.origin = {size / 2.0f, size / 2.0f, size / 2.0f}}};
}

SceneLoader::get_t generate_sphere(std::int32_t size, VoxelData::Type outer_value, VoxelData::Type core_value,
                                   bool noise = false, std::size_t seed = 0) {
    std::mt19937 rng(seed);

    auto mask = std::make_shared<ThresholdMask>(outer_value, true);
    auto transfer_function = std::make_shared<PiecewiseColourMapping>();

    VoxelData data(glm::ivec3{size, size, size});

    VoxelData::Type step = 50;

    std::size_t i = 0;
    while (outer_value + i * step < core_value) {
        transfer_function->add_point(glm::vec3{rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f},
                                     outer_value + i * step);
        i++;
    }
    transfer_function->add_point(glm::vec3{rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f},
                                 core_value);

    for (std::size_t z = 0; z < data.size().z; z++) {
        for (std::size_t y = 0; y < data.size().y; y++) {
            for (std::size_t x = 0; x < data.size().x; x++) {
                float const length = glm::length(glm::vec3{x, y, z} - (glm::vec3(size) / 2.0f));

                if (length > (size / 2.0f)) continue;

                float const normalized_distance = length / (size / 2.0f);

                data.set_voxel(glm::ivec3{x, y, z},
                               outer_value * normalized_distance + core_value * (1.0f - normalized_distance));
            }
        }
    }

    if (noise) add_noise(data, mask, (core_value - outer_value) / 10, 0.2f);

    return {data, mask, transfer_function,
            SceneLoader::SceneTransform{.origin = {size / 2.0f, size / 2.0f, size / 2.0f}}};
}

SceneLoader::get_t generate_random_cubes(std::int32_t size, std::size_t cube_count, std::size_t max_size,
                                         VoxelData::Type min_value, VoxelData::Type max_value, bool noise,
                                         std::size_t seed = 0) {
    std::mt19937 rng(seed);

    auto mask = std::make_shared<ThresholdMask>(min_value, true);
    auto transfer_function = std::make_shared<PiecewiseColourMapping>();

    transfer_function->add_point(glm::vec3{rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f},
                                 min_value);
    transfer_function->add_point(glm::vec3{rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f, rng() % 1024 / 1024.0f},
                                 max_value);

    VoxelData data(glm::ivec3{size, size, size});

    for (std::size_t i = 0; i < cube_count; i++) {
        glm::ivec3 const start = glm::ivec3{rng() % size, rng() % size, rng() % size};
        glm::ivec3 const cube_size = glm::ivec3{1 + rng() % max_size, 1 + rng() % max_size, 1 + rng() % max_size};

        glm::ivec3 const end = glm::min(start + cube_size, glm::ivec3(size));

        VoxelData::Type const value = min_value + rng() % (max_value - min_value + 1);

        for (std::size_t x = start.x; x < end.x; x++) {
            for (std::size_t y = start.y; y < end.y; y++) {
                for (std::size_t z = start.z; z < end.z; z++) {
                    data.set_voxel(glm::ivec3{x, y, z}, value);
                }
            }
        }
    }

    if (noise) add_noise(data, mask, (max_value - min_value) / 10, 0.2f);

    return {data, mask, transfer_function,
            SceneLoader::SceneTransform{.origin = {size / 2.0f, size / 2.0f, size / 2.0f}}};
}

};  // namespace builtin

SceneLoader::SceneLoader() {
    m_names.push_back("Default");
    m_names.push_back("Random");
    m_names.push_back("Menger sponge");
    m_names.push_back("Sphere");
    m_names.push_back("Cubes");

    for (auto&& entry : std::filesystem::directory_iterator("data")) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::ifstream file(entry.path());
            glz::basic_istream_buffer<std::ifstream> buffer(file);

            m_scenes.push_back({});
            auto ec = glz::read_json<Scene>(m_scenes.back(), buffer);

            if (ec) {
                spdlog::error("{} (code: {})", ec.custom_error_message, std::to_underlying(ec.ec));
            }

            m_names.push_back(m_scenes.back().name.c_str());
            spdlog::info("Added dataset: {}", m_names.back());
        }
    }
}

SceneLoader::get_t SceneLoader::get(std::int32_t index, std::int32_t size, bool noise) {
    if (index < std::to_underlying(BultinScenes::COUNT)) {
        switch ((BultinScenes)index) {
            case BultinScenes::DEFAULT:
                return builtin::generate_default_scene();
            case BultinScenes::RANDOM:
                return builtin::generate_random(size);
            case BultinScenes::MENGER:
                return builtin::generate_menger_sponge(size, 0, 2000, noise);
            case BultinScenes::SPHERE:
                return builtin::generate_sphere(size, 0, 2000, noise);
            case BultinScenes::CUBES:
                return builtin::generate_random_cubes(size, size * 2, size / 4, 0, 2000, noise);
            case BultinScenes::COUNT:
                break;
        }
    }

    index -= std::to_underlying(BultinScenes::COUNT);

    Scene& scene = m_scenes[index];
    VoxelData data;
    std::shared_ptr<ITransferFunction> transfer_function;
    std::shared_ptr<IMask> mask;
    SceneTransform transform;

    if (scene.format == "vl32") {
        auto [_data, _transfer_function] =
            loaders::read_vl32("data/" + scene.file, glm::ivec3{scene.size[0], scene.size[1], scene.size[2]});
        mask = std::make_shared<ThresholdMask>(0, true);
        data = _data;
        transfer_function = _transfer_function;
    } else {
        auto _data = loaders::read_raw("data/" + scene.file, glm::ivec3{scene.size[0], scene.size[1], scene.size[2]});
        mask = std::make_shared<ThresholdMask>(scene.threshold + scene.value_offset, true);
        data = _data;

        if (scene.colour_map.type == "piecewise") {
            auto _transfer_function = std::make_shared<PiecewiseColourMapping>();

            for (auto const& colour_value : scene.colour_map.values) {
                _transfer_function->add_point(
                    glm::vec3{colour_value.colour[0], colour_value.colour[1], colour_value.colour[2]},
                    colour_value.value + scene.value_offset);
            }
            transfer_function = _transfer_function;
        } else {
            auto _transfer_function = std::make_shared<ValueColourMapping>();

            for (auto const& colour_value : scene.colour_map.values) {
                _transfer_function->set_colour(
                    colour_value.value + scene.value_offset,
                    glm::vec3{colour_value.colour[0], colour_value.colour[1], colour_value.colour[2]});
            }
            transfer_function = _transfer_function;
        }
    }

    transform.rotation = {scene.transform.rotation[0], scene.transform.rotation[1], scene.transform.rotation[2]};
    transform.origin = {scene.transform.origin[0], scene.transform.origin[1], scene.transform.origin[2]};
    transform.scale = {scene.transform.scale[0], scene.transform.scale[1], scene.transform.scale[2]};
    transform.position = {scene.transform.position[0], scene.transform.position[1], scene.transform.position[2]};

    return {data, mask, transfer_function, transform};
}

char const* const* SceneLoader::names() const {
    return m_names.data();
}

std::int32_t SceneLoader::count() const {
    return m_names.size();
}
