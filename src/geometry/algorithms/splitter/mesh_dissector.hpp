#pragma once

#include <cstdint>
#include <geometry/math/vec3.hpp>
#include <geometry/mesh/mesh.hpp>
#include <vector>

namespace geometry::algorithms::splitter
{
    using Mesh = geometry::mesh::Mesh;
    using Vec3 = geometry::math::Vec3;

    /**
     * @brief Represents a cutting operation to be performed on a mesh.
     *
     * Each CutOrder specifies:
     * - The target mesh to cut.
     * - A point on the cutting plane.
     * - The normal of the cutting plane.
     * - A priority value indicating the order in which cuts should be applied.
     */
    struct CutOrder
    {
        Mesh* target;
        Vec3  planePoint;
        Vec3  planeNormal;
        int   priority; // order of execution
    };

    enum class Strategy : std::uint8_t
    {
        BIGGER_PIECES  = 0, // fewer, larger pieces (fill the maxPieceSize box as much as possible before cutting)
        SMALLER_PIECES = 1, // more, smaller pieces (cut as soon as any dimension exceeds maxPieceSize)
    };

    /**
     * @brief Configuration for the MeshDissector, specifying how to split a mesh into smaller pieces.
     */
    struct DissectorConfig
    {
        Vec3     maxPieceSize; // mm
        Strategy strategy;     // cutting strategy
    };

    class MeshDissector
    {
      public:
        static std::vector<CutOrder> getCutOrder(Mesh& mesh, const DissectorConfig& config);
    };
} // namespace geometry::algorithms::splitter
