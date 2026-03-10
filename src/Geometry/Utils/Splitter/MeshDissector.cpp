#include <Geometry/Utils/MeshUtils.hpp>
#include <Geometry/Utils/Splitter/MeshDissector.hpp>
#include <Splinter3D/Utils/Logger.hpp>

namespace geometry::utils::splitter
{
    std::vector<CutOrder> MeshDissector::getCutOrder(Mesh& mesh, const DissectorConfig& config)
    {
        geometry::utils::MeshBounds bounds = geometry::utils::computeMeshBounds(mesh);
        if (!bounds.valid)
        {
            return {};
        }

        splinter3D::utils::Logger::getInstance().clog(
            "[MeshDissector] Mesh bounds: min(", bounds.min.x, ", ", bounds.min.y, ", ", bounds.min.z,
            ") max(", bounds.max.x, ", ", bounds.max.y, ", ", bounds.max.z, ")\n");

        std::vector<CutOrder> cutOrders;
        int                   nbCutX = static_cast<int>(std::ceil((bounds.max.x - bounds.min.x) / config.maxPieceSize.x)) - 1;
        int                   nbCutY = static_cast<int>(std::ceil((bounds.max.y - bounds.min.y) / config.maxPieceSize.y)) - 1;
        int                   nbCutZ = static_cast<int>(std::ceil((bounds.max.z - bounds.min.z) / config.maxPieceSize.z)) - 1;

        splinter3D::utils::Logger::getInstance().clog(
            "[MeshDissector] Cutting into grid of ", nbCutX + 1, " x ", nbCutY + 1, " x ", nbCutZ + 1,
            " = ", (nbCutX + 1) * (nbCutY + 1) * (nbCutZ + 1), " pieces\n");

        for (int i = 0; i < nbCutX; ++i)
        {
            Vec3 planePoint{
                bounds.min.x + (float) (i + 1) * config.maxPieceSize.x,
                bounds.min.y,
                bounds.min.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{1, 0, 0}, i});
        }
        for (int i = 0; i < nbCutY; ++i)
        {
            Vec3 planePoint{
                bounds.min.x,
                bounds.min.y + (float) (i + 1) * config.maxPieceSize.y,
                bounds.min.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{0, 1, 0}, nbCutX + i});
        }
        for (int i = 0; i < nbCutZ; ++i)
        {
            Vec3 planePoint{
                bounds.min.x,
                bounds.min.y,
                bounds.min.z + (float) (i + 1) * config.maxPieceSize.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{0, 0, 1}, nbCutX + nbCutY + i});
        }

        return cutOrders;
    }
} // namespace geometry::utils::splitter