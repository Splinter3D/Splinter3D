#pragma once

#include <Geometry/BBox.hpp>
#include <Geometry/Mesh.hpp>
#include <Renderer/IRenderer.hpp>
#include <algorithm>
#include <cmath>

namespace geometry::utils
{
    // Move the camera to frame the mesh
    void frameCameraOnMesh(renderer::IRenderer& renderer, const BBox& bounds);
} // namespace geometry::utils
