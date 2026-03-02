#pragma once

#include <Geometry/Mesh.hpp>
#include <Renderer/RaylibRenderer.hpp>
#include <algorithm>
#include <cmath>

namespace geometry::meshutils
{
    struct MeshBounds
    {
        geometry::Vec3 min{};
        geometry::Vec3 max{};
        bool           valid{false};
    };

    // Compute the bounding box of a mesh
    MeshBounds computeMeshBounds(const geometry::Mesh& mesh);

    // Compute center of the bounding box
    geometry::Vec3 computeBoundsCenter(const MeshBounds& bounds);

    // Compute radius of a bounding sphere around the mesh
    float computeBoundsRadius(const MeshBounds& bounds);

    // Move the camera to frame the mesh
    void frameCameraOnMesh(renderer::RaylibRenderer& renderer, const MeshBounds& bounds);
}