cmake_minimum_required(VERSION 4.2)

project(imgui)

file(GLOB SOURCES imgui*.cpp)
list(APPEND SOURCES backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp)

add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC backends .)
target_link_libraries(${PROJECT_NAME} PUBLIC glfw)
