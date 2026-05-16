#pragma once

#include <Geometry/Mesh.hpp>
#include <Renderer/IRenderer.hpp>
#include <algorithm>
#include <cmath>

namespace geometry
{
    struct BBox
    {

        BBox(const geometry::Mesh& mesh);

        BBox(const std::vector<geometry::Triangle> triangles);

        geometry::Vec3 getMax() const;
        geometry::Vec3 getMin() const;
        geometry::Vec3 getSize() const;
        geometry::Vec3 getCenter() const;

        float getRadius() const;
        bool  isValid() const;

      private:
        geometry::Vec3 min{};
        geometry::Vec3 max{};
        geometry::Vec3 size{};
        geometry::Vec3 center{};
        float          radius{0};
        bool           valid{false};

        void computeCenter();
        void computeSize();
        void computeRadius();

        void initializeFromTriangles(const std::vector<geometry::Triangle>
                                         triangles);
    };
} // namespace geometry
