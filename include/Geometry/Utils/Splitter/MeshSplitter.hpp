#pragma once
#include <Geometry/Mesh.hpp>
#include <utility>

namespace geometry::utils::splitter
{
    /**
     * @brief Splits a mesh into two watertight halves along an arbitrary plane.
     *
     * @details Algorithm overview  (BSP plane clipping)
     * For every triangle in the mesh we compute the signed distance of each
     * vertex from the cutting plane:
     *
     *
     * Vertices above  Result
     * -------------------------
     * 3               Triangle goes entirely to the "above" mesh
     * 0               Triangle goes entirely to the "below" mesh
     * 2               Triangle is clipped: one sub-triangle below, a quad (2 triangles) above
     * 1               Triangle is clipped: one sub-triangle above, a quad (2 triangles) below
     *
     * Every clipped triangle also emits one directed edge on the cut plane.
     * After processing all triangles, addCap() chains those edges into closed
     * polygon loops and triangulates them to seal both mesh halves.
     *
     * @param mesh   The source mesh to split (not modified).
     * @param point  Any point lying on the cutting plane.
     * @param normal The plane's outward normal — the "above" mesh is on this side.
     *               Does not need to be normalized.
     * @return       A pair {above, below} of watertight meshes.
     */
    std::pair<Mesh, Mesh> splitByPlane(const Mesh&           mesh,
                                       const geometry::Vec3& point,
                                       const geometry::Vec3& normal);
} // namespace geometry::utils::splitter