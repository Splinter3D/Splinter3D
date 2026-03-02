#pragma once

#include <Objects3D/OTriangle.hpp>
#include <Objects3D/OVec3.hpp>

#include <vector>
#include <string>

namespace objects3D
{
    struct OMesh {
        std::vector<OTriangle> triangles;

        OMesh() = default;
        OMesh(const std::vector<OTriangle>& tris) : triangles(tris) {}

        static OMesh fromSTL(const std::string& filename);
    };
}
