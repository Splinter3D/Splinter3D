#pragma once

#include <cstdint>
#include <Geometry/Mesh.hpp>
#include <Geometry/Vec3.hpp>
#include <cstdint>

namespace geometry::utils::splitter
{
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
} // namespace geometry::utils::splitter