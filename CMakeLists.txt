cmake_minimum_required(VERSION 4.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(voxel-rendering)

add_subdirectory(external)

set(SOURCES
    src/main.cpp
    src/VoxelData.cpp
    src/renderers/NoopRenderer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw glm spdlog imgui glaze::glaze Taskflow)
target_include_directories(${PROJECT_NAME} PRIVATE src)
