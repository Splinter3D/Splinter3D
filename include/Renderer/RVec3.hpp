#pragma once

namespace renderer
{
    struct RVec3
    {
        float x;
        float y;
        float z;

        RVec3() : x(0), y(0), z(0) {}
        RVec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

        RVec3 operator-(const RVec3& other) const
        {
            return RVec3(x - other.x, y - other.y, z - other.z);
        }

        static RVec3 cross(const RVec3& a, const RVec3& b)
        {
            return RVec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static float dotProduct(const RVec3& a, const RVec3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
    };
}
