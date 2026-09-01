cmake_minimum_required(VERSION 4.2)

include(FetchContent)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)

option(GLM_BUILD_TESTS OFF)
option(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(glm)
