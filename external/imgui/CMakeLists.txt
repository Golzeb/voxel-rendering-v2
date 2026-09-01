cmake_minimum_required(VERSION 4.2)

include(FetchContent)

FetchContent_Populate(
    imgui
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/imgui-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/imgui-build
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.8
    GIT_SHALLOW 1
)


if(NOT EXISTS ${imgui_SOURCE_DIR}/CMakeLists.txt)
    file(COPY ${CMAKE_CURRENT_LIST_DIR}/custom/CMakeLists.txt DESTINATION ${imgui_SOURCE_DIR})
endif()

add_subdirectory(${imgui_SOURCE_DIR} ${imgui_BINARY_DIR})
