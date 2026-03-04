#pragma once

#include <Geometry/Vec3.hpp>
#include <raymath.h>

namespace objects3D
{
    /**
     * Represents the position, rotation, and scale of an Object3D.
     */
    struct Transform
    {
        geometry::Vec3 position{0, 0, 0};
        geometry::Vec3 rotation{0, 0, 0};
        geometry::Vec3 scale{1, 1, 1};

        inline Matrix toMatrix() const
        {
            return MatrixScale(scale.x, scale.y, scale.z) *
                   MatrixRotateXYZ(rotation.toRaylib()) *
                   MatrixTranslate(position.x, position.y, position.z);
        }
    };
} // namespace objects3D
