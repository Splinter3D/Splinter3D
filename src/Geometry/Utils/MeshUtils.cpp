#include <Geometry/Utils/MeshUtils.hpp>
#include <raymath.h> // for Vector3 operations

namespace geometry::meshutils
{
    // -----------------------------
    // Mesh bounds computations
    // -----------------------------

    MeshBounds computeMeshBounds(const geometry::Mesh& mesh)
    {
        MeshBounds bounds;
        for (const auto& tri : mesh.triangles)
        {
            for (const auto& v : tri.vertices)
            {
                if (!bounds.valid)
                {
                    bounds.min   = v;
                    bounds.max   = v;
                    bounds.valid = true;
                }
                else
                {
                    bounds.min.x = std::min(bounds.min.x, v.x);
                    bounds.min.y = std::min(bounds.min.y, v.y);
                    bounds.min.z = std::min(bounds.min.z, v.z);

                    bounds.max.x = std::max(bounds.max.x, v.x);
                    bounds.max.y = std::max(bounds.max.y, v.y);
                    bounds.max.z = std::max(bounds.max.z, v.z);
                }
            }
        }
        return bounds;
    }

    geometry::Vec3 computeBoundsCenter(const MeshBounds& bounds)
    {
        return geometry::Vec3{
            (bounds.min.x + bounds.max.x) * 0.5f,
            (bounds.min.y + bounds.max.y) * 0.5f,
            (bounds.min.z + bounds.max.z) * 0.5f};
    }

    float computeBoundsRadius(const MeshBounds& bounds)
    {
        float dx = bounds.max.x - bounds.min.x;
        float dy = bounds.max.y - bounds.min.y;
        float dz = bounds.max.z - bounds.min.z;
        return 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    // -----------------------------
    // Camera framing
    // -----------------------------

    void frameCameraOnMesh(renderer::RaylibRenderer& renderer, const MeshBounds& bounds)
    {
        if (!bounds.valid)
            return;

        geometry::Vec3 center = computeBoundsCenter(bounds);
        renderer.setCameraTarget(center);

        float radius   = std::max(computeBoundsRadius(bounds), 0.5f);
        float distance = radius * 2.0f;

        renderer.setOrbitDistance(distance);
        renderer.setOrbitAngles(0.8f, 0.6f);
    }
} // namespace geometry::meshutils