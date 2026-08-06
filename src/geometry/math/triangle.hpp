#pragma once

#include <geometry/math/vec3.hpp>

namespace geometry::math
{
    /**
     * @brief Represents a triangle in 3D space, defined by three vertices.
     */
    struct Triangle
    {
        Vec3 vertices[3];
    };
} // namespace geometry::math
