#pragma once

#include <algorithm>
#include <cmath>
#include <geometry/mesh/mesh.hpp>
#include <vector>

namespace geometry::algorithms
{
    struct MeshBounds
    {
        geometry::math::Vec3 min{};
        geometry::math::Vec3 max{};
        geometry::math::Vec3 size{};
        bool                 valid{false};
    };

    // Compute the bounding box of a mesh
    MeshBounds computeMeshBounds(const geometry::mesh::Mesh& mesh);

    // Compute center of the bounding box
    geometry::math::Vec3 computeBoundsCenter(const MeshBounds& bounds);

    // Compute radius of a bounding sphere around the mesh
    float computeBoundsRadius(const MeshBounds& bounds);
} // namespace geometry::algorithms
