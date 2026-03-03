#pragma once

#include <Geometry/Vec3.hpp>

namespace objects3D
{
    struct Transform
    {
        geometry::Vec3 position{0, 0, 0};
        geometry::Vec3 rotation{0, 0, 0};
        geometry::Vec3 scale{1, 1, 1};
    };
} // namespace objects3D