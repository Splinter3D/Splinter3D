#pragma once

#include <Renderer/RVec3.hpp>
#include <Renderer/Color.hpp>
#include <Renderer/RTriangle.hpp>

#include <Objects3D/OMesh.hpp>
#include <Objects3D/OTriangle.hpp>
#include <Objects3D/OVec3.hpp>

#include <vector>

namespace renderer
{
    struct RMesh
    {
        std::vector<RTriangle> triangles;

        RMesh() = default;
        RMesh(const std::vector<RTriangle>& tris) : triangles(tris) {}

        RMesh makeRenderable(const objects3D::OMesh& mesh, Color color);
    };
}
