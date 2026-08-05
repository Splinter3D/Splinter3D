#pragma once
#include <geometry/math/vec3.hpp>

namespace geometry::math
{
    /**
     * @brief Represents a ray in 3D space, defined by an origin point and a direction vector.
     */
    struct Ray
    {
        Vec3 origin;
        Vec3 direction;
    };
} // namespace geometry::math
