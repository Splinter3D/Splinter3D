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
add_library(prototype_renderer SHARED
    renderer/src/RaylibRenderer.cpp
)

target_include_directories(prototype_renderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/renderer/include
)

target_link_libraries(prototype_renderer
    PUBLIC
        raylib
        glfw
)

add_library(prototype::renderer ALIAS prototype_renderer)


# =========================
# Objects3D library
# =========================
add_library(prototype_objects3D SHARED
    objects3D/src/OMesh.cpp
)

target_include_directories(prototype_objects3D
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/objects3D/include
)

add_library(prototype::objects3D ALIAS prototype_objects3D)

# =========================
# Main executable
# =========================
add_executable(Prototype
    src/main.cpp
)

target_link_libraries(Prototype
    PRIVATE
        prototype::renderer
        prototype::objects3D
)

target_compile_options(Prototype PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)