cmake_minimum_required(VERSION 3.16)

project(Prototype LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# =========================
# Dependencies (vcpkg)
# =========================
find_package(raylib CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)


# =========================
# Renderer library
# =========================
add_library(protoype_renderer SHARED
    renderer/src/RaylibRenderer.cpp
)

target_include_directories(protoype_renderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer/include
)

target_link_libraries(protoype_renderer
    PUBLIC
        raylib
        glfw
)

add_library(protoype::renderer ALIAS protoype_renderer)


# =========================
# Main executable
# =========================
add_executable(Prototype
    src/main.cpp
)

target_link_libraries(Prototype
    PRIVATE
        protoype::renderer
)

target_compile_options(Prototype PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)