#pragma once

#include "renderer/RVec3.hpp"
#include "renderer/Color.hpp"

#include "objects3D/OMesh.hpp"
#include "objects3D/OTriangle.hpp"
#include "objects3D/OVec3.hpp"

#include <vector>

namespace renderer
{
    struct RMesh
    {
        std::vector<RTriangle> triangles;

        RMesh() = default;
        RMesh(const std::vector<RTriangle>& tris) : triangles(tris) {}

        RMesh makeRenderable(const Object3D::OMesh& mesh, Color color);
    };
}