#pragma once

// Must come before any OCCT header (see the file).
#include "geometry/occt/shape.hpp"
#include "geometry/occt/windows_prelude.hpp"

#include <Poly_Triangulation.hxx>
#include <cstdint>
#include <gp_Lin.hxx>
#include <gp_Trsf.hxx>
#include <gp_XYZ.hxx>
#include <memory>
#include <optional>
#include <vector>

namespace geometry::occt
{
    /**
     * @brief Finds where a line meets a shape's triangles, fast: a bounding
     * volume tree over every triangle of its faces' triangulations.
     *
     * Building it reads the whole mesh (seconds for tens of millions of
     * triangles), so it is meant for a background thread; the shape's
     * triangulations must not change meanwhile (they never do: a new mesh
     * is a new triangulation). Queries are cheap and const, so any thread
     * may run them once it is built.
     */
    class MeshPickIndex
    {
      public:
        /** @brief The index of shape's triangles, in its own coordinates; nullptr if it has none. */
        [[nodiscard]] static std::shared_ptr<const MeshPickIndex> build(const Shape& shape);

        /**
         * @brief Parameter along ray (gp_Lin::Location() + t * Direction())
         * of the triangle hit first coming from far back along the direction,
         * i.e. the smallest t over all hits (it may be negative); empty if
         * the line misses. Edges and corners count as hits.
         */
        [[nodiscard]] std::optional<double> intersect(const gp_Lin& ray) const;

      private:
        struct Face
        {
            occ::handle<Poly_Triangulation> mesh;
            gp_Trsf                         location;
            bool                            moved = false;
            std::uint32_t                   first = 0; // its first triangle's id
        };

        // A box around triangles_[start, start + count) for a leaf; for an
        // inner node (count == 0) its children are the next node and `right`.
        struct Node
        {
            float         min[3];
            float         max[3];
            std::uint32_t start = 0;
            std::uint32_t count = 0;
            std::uint32_t right = 0;
        };

        void   corners(std::uint32_t triangle, gp_XYZ& a, gp_XYZ& b, gp_XYZ& c) const;
        void   buildNode(std::uint32_t start, std::uint32_t count, const std::vector<float>& centroids);
        double hitTriangle(std::uint32_t triangle, const gp_XYZ& origin, const gp_XYZ& direction, bool& hit) const;

        std::vector<Face>          faces_;
        std::vector<std::uint32_t> triangles_; // ids, in leaf order
        std::vector<Node>          nodes_;
    };
} // namespace geometry::occt
