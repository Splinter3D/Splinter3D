#pragma once

#include <geometry/math/constants/constants.hpp>

namespace geometry::math
{
    /**
     * @brief Represents a 3D vector or point in space.
     */
    struct Vec3
    {
        float x;
        float y;
        float z;

        Vec3()
            : x(0), y(0), z(0)
        { }

        Vec3(float X, float Y, float Z)
            : x(X), y(Y), z(Z)
        { }

        Vec3 operator-(const Vec3& other) const
        {
            return Vec3(x - other.x, y - other.y, z - other.z);
        }

        Vec3 operator+(const Vec3& other) const
        {
            return Vec3(x + other.x, y + other.y, z + other.z);
        }

        static Vec3 cross(const Vec3& a, const Vec3& b)
        {
            return Vec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x);
        }

        static float dotProduct(const Vec3& a, const Vec3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        /**
         * @brief Converts the Vec3 to a Raylib Vector3.
         * @deprecated This method is deprecated due to us wanting to remove all the raylib dependencies from the geometry library.
         */
        // inline Vector3 toRaylib() const
        // {
        //     return Vector3{x, y, z};
        // }

        inline Vec3 toRadian() const
        {
            return Vec3{
                x * (geometry::math::constants::PI / 180.0f),
                y * (geometry::math::constants::PI / 180.0f),
                z * (geometry::math::constants::PI / 180.0f)};
        }
    };
} // namespace geometry::math
