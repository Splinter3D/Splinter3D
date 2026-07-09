#include <Geometry/Utils/Splitter/MeshDissector.hpp>
#include <Geometry/Utils/Splitter/MeshSplitter.hpp>
#include <Splinter3D/Utils/Logger.hpp>
#include <cmath>

namespace geometry::utils::splitter
{
#pragma region Private helper functions

    /**
     * @brief Computes the signed distance from a point to a plane.
     *
     * The plane is defined by a point on it and its outward normal.
     * The sign tells which side of the plane the point is on:
     * - Positive → same side as the normal ("above")
     * - Negative → opposite side ("below")
     * - Zero     → exactly on the plane
     *
     * @param v          The point to test.
     * @param planePoint Any point lying on the plane.
     * @param normal     The plane's outward normal (does not need to be normalized).
     * @return           Signed distance from @p v to the plane.
     */
    static float planeDist(const Vec3& v, const Vec3& planePoint, const Vec3& normal)
    {
        return (v.x - planePoint.x) * normal.x +
               (v.y - planePoint.y) * normal.y +
               (v.z - planePoint.z) * normal.z;
    }

    /**
     * @brief Linearly interpolates the exact crossing point of edge (a→b) with a plane.
     *
     * Given the signed distances of each endpoint from the plane, computes
     * the parameter t ∈ [0,1] such that a + t*(b-a) lies exactly on the plane,
     * then returns that point.
     *
     * @param a  Start of the edge.
     * @param b  End of the edge.
     * @param dA Signed distance of @p a from the plane (from planeDist()).
     * @param dB Signed distance of @p b from the plane (from planeDist()).
     * @return   The point on the plane where the edge crosses.
     */
    static Vec3 planeIntersect(const Vec3& a, const Vec3& b, float dA, float dB)
    {
        float t = dA / (dA - dB); // t=0 → a, t=1 → b
        return {
            a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y),
            a.z + t * (b.z - a.z)};
    }

    /**
     * @brief Appends a triangle (a, b, c) to a mesh.
     *
     * Winding order is preserved as given — callers are responsible
     * for passing vertices in the correct order for their desired face normal.
     *
     * @param mesh The mesh to append to.
     * @param a    First vertex.
     * @param b    Second vertex.
     * @param c    Third vertex.
     */
    static void addTriangle(Mesh& mesh, const Vec3& a, const Vec3& b, const Vec3& c)
    {
        Triangle tri;
        tri.vertices[0] = a;
        tri.vertices[1] = b;
        tri.vertices[2] = c;
        mesh.triangles.push_back(tri);
    }

    /**
     * @brief Returns true if two Vec3 are within eps of each other on all axes.
     *
     * Used to match cut edge endpoints that should be identical but may differ
     * slightly due to floating-point arithmetic.
     *
     * @param a   First vector.
     * @param b   Second vector.
     * @param eps Tolerance (default 1e-5).
     * @return    True if the vectors are within epsilon on every axis.
     */
    static bool vec3Near(const Vec3& a, const Vec3& b, float eps = 1e-5f)
    {
        return std::abs(a.x - b.x) < eps &&
               std::abs(a.y - b.y) < eps &&
               std::abs(a.z - b.z) < eps;
    }

#pragma endregion
#pragma region Cap generation

    // TODO switch from centroid fan to constrained Delaunay triangulation for better handling of concave cross-sections and holes

    /**
     * @brief Fills the open cross-section left by splitByPlane() with cap triangles.
     *
     * @details Algorithm overview
     * Every triangle clipped by the cutting plane contributes exactly one directed
     * edge that lies on that plane.  Chaining those edges end-to-start reconstructs
     * the boundary polygon(s) of the cross-section — the same idea used in
     * half-edge mesh data structures and Blender's Bisect operation.
     *
     * Once each closed loop is found, a **centroid fan** triangulates it:
     * the centroid of the loop is connected to every consecutive pair of loop
     * vertices, producing one triangle per edge.
     *
     * The cut plane's normal is used to determine the correct winding for each
     * mesh half so that normals point outward on both sides.
     *
     * @note For concave cross-sections the centroid fan may produce self-intersecting
     *       triangles.  A full ear-clipping triangulator would be needed for those cases.
     *
     * @param above     The "above" half-mesh — cap triangles facing away from normal are added here.
     * @param below     The "below" half-mesh — cap triangles facing toward normal are added here.
     * @param cutEdges  Directed edges collected during clipping (consumed by this function).
     * @param normal    The cut plane's outward normal.
     */
    static void addCap(Mesh& above, Mesh& below,
                       std::vector<std::pair<Vec3, Vec3>> cutEdges,
                       const Vec3&                        normal)
    {
        if (cutEdges.empty())
            return;

        // ── Step 1: chain directed edges into closed polygon loops ──────────────
        //
        // Each edge is (start → end).  We look for the next edge whose start
        // matches the current end, within floating-point tolerance.
        // This produces one loop per connected boundary contour on the cut plane.
        // Example:
        //     Input cutEdges: [(A→B), (C→D), (B→E), (D→A), (G→T), (T→U), (U→G), (E→C)]
        //     Output loops:  [[A, B, E, C, D], [G, T, U]]
        std::vector<std::vector<Vec3>> loops;

        while (!cutEdges.empty())
        {
            std::vector<Vec3> loop;
            loop.push_back(cutEdges[0].first);
            Vec3 current = cutEdges[0].second;
            cutEdges.erase(cutEdges.begin());

            bool advanced = true;
            while (advanced && !vec3Near(current, loop[0]))
            {
                advanced = false;
                for (size_t i = 0; i < cutEdges.size(); ++i)
                {
                    if (vec3Near(cutEdges[i].first, current))
                    {
                        loop.push_back(current);
                        current = cutEdges[i].second;
                        cutEdges.erase(cutEdges.begin() + static_cast<long>(i));
                        advanced = true;
                        break;
                    }
                }
            }

            if (loop.size() >= 3)
                loops.push_back(loop);
        }

        // ── Step 2: centroid-fan triangulate each loop ───────────────────────────
        //
        // For each consecutive pair (a, b) in the loop, we emit a triangle
        // (centroid, a, b) or (centroid, b, a) depending on which way the
        // resulting face normal points relative to the cut normal.
        // The two mesh halves always receive opposite windings so both are closed.
        for (const auto& loop : loops)
        {
            // Centroid of the loop — the hub of the fan
            Vec3 centroid{0, 0, 0};
            for (const auto& v : loop)
            {
                centroid.x += v.x;
                centroid.y += v.y;
                centroid.z += v.z;
            }
            const float inv = 1.0f / static_cast<float>(loop.size());
            centroid.x *= inv;
            centroid.y *= inv;
            centroid.z *= inv;

            for (size_t i = 0; i < loop.size(); ++i)
            {
                const Vec3& a = loop[i];
                const Vec3& b = loop[(i + 1) % loop.size()];

                // Compute the face normal of candidate triangle (a, b, centroid)
                // to decide whether it points toward or away from the cut normal.
                Vec3 ab         = {b.x - a.x, b.y - a.y, b.z - a.z};
                Vec3 ac         = {centroid.x - a.x, centroid.y - a.y, centroid.z - a.z};
                Vec3 faceNormal = {
                    ab.y * ac.z - ab.z * ac.y,
                    ab.z * ac.x - ab.x * ac.z,
                    ab.x * ac.y - ab.y * ac.x};

                // Positive dot → face normal aligns with cut normal
                float dot = faceNormal.x * normal.x +
                            faceNormal.y * normal.y +
                            faceNormal.z * normal.z;

                if (dot > 0.0f)
                {
                    addTriangle(above, centroid, b, a); // reversed winding for above
                    addTriangle(below, centroid, a, b); // natural winding for below
                }
                else
                {
                    addTriangle(above, centroid, a, b); // natural winding for above
                    addTriangle(below, centroid, b, a); // reversed winding for below
                }
            }
        }
    }

#pragma endregion
#pragma region Split by plane

    std::pair<Mesh, Mesh> splitByPlane(const Mesh& mesh, const Vec3& point, const Vec3& normal)
    {
        Mesh                               above, below; // output meshes for the two halves
        std::vector<std::pair<Vec3, Vec3>> cutEdges;     // edges along the cut plane, used to build the cap

        // splinter3D::utils::Logger::getInstance().clog("[MeshSplitter] Splitting ", mesh.triangles.size(), " triangles\n");

        for (const auto& tri : mesh.triangles)
        {
            const Vec3& A = tri.vertices[0];
            const Vec3& B = tri.vertices[1];
            const Vec3& C = tri.vertices[2];

            float dA = planeDist(A, point, normal);
            float dB = planeDist(B, point, normal);
            float dC = planeDist(C, point, normal);

            bool aAbove = dA >= 0.0f;
            bool bAbove = dB >= 0.0f;
            bool cAbove = dC >= 0.0f;
            int  n      = (aAbove ? 1 : 0) + (bAbove ? 1 : 0) + (cAbove ? 1 : 0);

            // This triangle is entirely ABOVE the plane, keep as-is
            if (n == 3)
            {
                above.triangles.push_back(tri);
                continue;
            }
            // This triangle is entirely BELOW the plane, keep as-is
            if (n == 0)
            {
                below.triangles.push_back(tri);
                continue;
            }

            // ── Clipping cases ───────────────────────────────────────────────────
            // Reorder vertices so v0 is always the "lone" vertex:
            //   n==2 → v0 is the single vertex BELOW the plane
            //   n==1 → v0 is the single vertex ABOVE the plane
            // v1 and v2 are the remaining two vertices (on the other side).
            // This gives a consistent layout for both clipping cases.
            Vec3  v0, v1, v2;
            float d0, d1, d2;

            if (n == 2)
            {
                // Identify the lone vertex below and keep the order A - B - C for consistent winding
                if (!aAbove)
                {
                    v0 = A;
                    d0 = dA;
                    v1 = B;
                    d1 = dB;
                    v2 = C;
                    d2 = dC;
                }
                else if (!bAbove)
                {
                    v0 = B;
                    d0 = dB;
                    v1 = C;
                    d1 = dC;
                    v2 = A;
                    d2 = dA;
                }
                else
                {
                    v0 = C;
                    d0 = dC;
                    v1 = A;
                    d1 = dA;
                    v2 = B;
                    d2 = dB;
                }

                // i1 and i2 are the two intersection points where the triangle edges cross the plane
                Vec3 i1 = planeIntersect(v0, v1, d0, d1);
                Vec3 i2 = planeIntersect(v0, v2, d0, d2);

                addTriangle(below, v0, i1, i2); // one triangle below
                addTriangle(above, v1, v2, i2); // quad above → two triangles
                addTriangle(above, v1, i2, i1);

                // Edge direction: i1→i2 (will chain with n==1 edges to close the loop)
                cutEdges.emplace_back(i1, i2);
            }
            else // n == 1
            {
                // Identify the lone vertex above
                if (aAbove)
                {
                    v0 = A;
                    d0 = dA;
                    v1 = B;
                    d1 = dB;
                    v2 = C;
                    d2 = dC;
                }
                else if (bAbove)
                {
                    v0 = B;
                    d0 = dB;
                    v1 = C;
                    d1 = dC;
                    v2 = A;
                    d2 = dA;
                }
                else
                {
                    v0 = C;
                    d0 = dC;
                    v1 = A;
                    d1 = dA;
                    v2 = B;
                    d2 = dB;
                }

                Vec3 i1 = planeIntersect(v0, v1, d0, d1);
                Vec3 i2 = planeIntersect(v0, v2, d0, d2);

                addTriangle(above, v0, i1, i2); // one triangle above
                addTriangle(below, v1, v2, i2); // quad below → two triangles
                addTriangle(below, v1, i2, i1);

                // Edge direction: i2→i1 (opposite of n==2 so loops close correctly)
                cutEdges.emplace_back(i2, i1);
            }
        }

        // Seal both mesh halves by filling the cross-section polygon(s)
        addCap(above, below, cutEdges, normal);

        // splinter3D::utils::Logger::getInstance().clog(
        //     "[MeshSplitter] Done — above: ", above.triangles.size(),
        //     " below: ", below.triangles.size(), " triangles\n");

        return {above, below};
    }

    std::vector<Mesh> splitByGrid(const Mesh& mesh, const Vec3& cellSize)
    {
        const DissectorConfig config{cellSize, Strategy::BIGGER_PIECES};
        auto                  cutOrders = MeshDissector::getCutOrder(const_cast<Mesh&>(mesh), config);

        std::vector<Mesh> pieces{mesh};

        for (const auto& cut : cutOrders)
        {
            std::vector<Mesh> newPieces;

            for (const auto& piece : pieces)
            {
                auto [above, below] = splitByPlane(piece, cut.planePoint, cut.planeNormal);
                if (!above.triangles.empty())
                    newPieces.push_back(above);
                if (!below.triangles.empty())
                    newPieces.push_back(below);
            }

            pieces = std::move(newPieces);
        }
        for (const auto& cut : cutOrders)
        {
            splinter3D::utils::Logger::getInstance().clog(
                "[MeshSplitter] Cut at plane (", cut.planePoint.x, ", ", cut.planePoint.y, ", ", cut.planePoint.z,
                ") with normal (", cut.planeNormal.x, ", ", cut.planeNormal.y, ", ", cut.planeNormal.z,
                ") → pieces: ", pieces.size(), "\n");
        }
        return pieces;
    }

} // namespace geometry::utils::splitter
