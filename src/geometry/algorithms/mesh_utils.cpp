#include <geometry/algorithms/mesh_utils.hpp>

namespace geometry::algorithms
{
    // -----------------------------
    // Mesh bounds computations
    // -----------------------------

    MeshBounds computeMeshBounds(const geometry::mesh::Mesh& mesh)
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
        bounds.size = geometry::math::Vec3{
            bounds.max.x - bounds.min.x,
            bounds.max.y - bounds.min.y,
            bounds.max.z - bounds.min.z};
        return bounds;
    }

    geometry::math::Vec3 computeBoundsCenter(const MeshBounds& bounds)
    {
        return geometry::math::Vec3{
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

} // namespace geometry::algorithms
