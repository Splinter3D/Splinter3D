#pragma once

namespace objects3D
{
    struct OVec3
    {
        float x;
        float y;
        float z;

        OVec3() : x(0), y(0), z(0) {}
        OVec3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

        OVec3 operator-(const OVec3& other) const
        {
            return OVec3(x - other.x, y - other.y, z - other.z);
        }

        static OVec3 cross(const OVec3& a, const OVec3& b)
        {
            return OVec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static float dotProduct(const OVec3& a, const OVec3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
    };
}