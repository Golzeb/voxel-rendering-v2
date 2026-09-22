cmake_minimum_required(VERSION 4.2)

set(CMAKE_EXPORT_COMPILE_COMMANDS True)

project(voxel-rendering)

add_subdirectory(external)

set(SOURCES
    src/main.cpp
    src/utils.cpp
    src/loaders.cpp
    src/VoxelData.cpp
    src/Camera.cpp
    src/MeshingPipeline.cpp
    src/gui/controls.cpp

    # Masks 
    src/masks/ThresholdMask.cpp
    
    # Providers
    src/data_providers/FullDataProvider.cpp
    src/data_providers/ChunkedDataProvider.cpp

    # Transfer functions
    src/transfer_functions/ValueColourMapping.cpp

    # Extractors
    src/extractors/NaiveMeshExtractor.cpp
    src/extractors/FaceCulledGreedyMeshExtractor.cpp
    src/extractors/FaceCulledMeshExtractor.cpp

    # Optimizers
    src/optimizers/MeshIndexer.cpp
    src/optimizers/NoopMeshOptimizer.cpp

    # Renderers
    src/renderers/NoopRenderer.cpp
    src/renderers/RasterRenderer.cpp
    src/renderers/InstancedRasterRenderer.cpp
    src/renderers/RayMarchingRenderer.cpp
    src/renderers/GridHierarchyRayMarchingRenderer.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_link_libraries(${PROJECT_NAME} PRIVATE glad glfw glm spdlog imgui glaze::glaze Taskflow)
target_include_directories(${PROJECT_NAME} PRIVATE src)
