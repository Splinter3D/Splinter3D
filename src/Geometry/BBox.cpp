#include "Geometry/BBox.hpp"

namespace geometry
{
    BBox::BBox(const geometry::Mesh& mesh)
    {
        initializeFromTriangles(mesh.triangles);
        computeCenter();
        computeSize();
        computeRadius();
    }

    BBox::BBox(const std::vector<geometry::Triangle> triangles)
    {
        initializeFromTriangles(triangles);
        computeCenter();
        computeSize();
        computeRadius();
    }

    geometry::Vec3 BBox::getMax() const
    {
        return max;
    }

    geometry::Vec3 BBox::getMin() const
    {
        return min;
    }

    geometry::Vec3 BBox::getSize() const
    {
        return size;
    }

    geometry::Vec3 BBox::getCenter() const
    {
        return center;
    }

    bool BBox::isValid() const
    {
        return valid;
    }

    float BBox::getRadius() const
    {
        return radius;
    }

    void BBox::initializeFromTriangles(const std::vector<geometry::Triangle> triangles)
    {
        for (const auto tri : triangles)
        {
            for (const auto v : tri.vertices)
            {
                if (!valid)
                {
                    min   = v;
                    max   = v;
                    valid = true;
                }
                else
                {
                    min.x = std::min(min.x, v.x);
                    min.y = std::min(min.y, v.y);
                    min.z = std::min(min.z, v.z);

                    max.x = std::max(max.x, v.x);
                    max.y = std::max(max.y, v.y);
                    max.z = std::max(max.z, v.z);
                }
            }
        }
    }

    void BBox::computeRadius()
    {
        float dx = max.x - min.x;
        float dy = max.y - min.y;
        float dz = max.z - min.z;
        radius   = 0.5f * std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    void BBox::computeCenter()
    {
        center = Vec3{
            (min.x + max.x) * 0.5f,
            (min.y + max.y) * 0.5f,
            (min.z + max.z) * 0.5f};
    }

    void BBox::computeSize()
    {
        size = Vec3{
            max.x - min.x,
            max.y - min.y,
            max.z - min.z};
    }
} // namespace geometry