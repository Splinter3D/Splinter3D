#include <Geometry/BBox.hpp>
#include <Geometry/Utils/Splitter/MeshDissector.hpp>
#include <Splinter3D/Utils/Logger.hpp>

namespace geometry::utils::splitter
{
    std::vector<CutOrder> MeshDissector::getCutOrder(Mesh& mesh, const DissectorConfig& config)
    {
        geometry::BBox bbox = geometry::BBox(mesh);
        if (!bbox.isValid() || config.maxPieceSize.x <= 0 || config.maxPieceSize.y <= 0 || config.maxPieceSize.z <= 0)
        {
            return {};
        }
        geometry::Vec3 bboxMin = bbox.getMin();
        geometry::Vec3 bboxMax = bbox.getMax();

        splinter3D::utils::Logger::getInstance().clog(
            "[MeshDissector] Mesh bbox: min(", bboxMin.x, ", ", bboxMin.y, ", ", bboxMin.z,
            ") max(", bboxMax.x, ", ", bboxMax.y, ", ", bboxMax.z, ")\n");

        std::vector<CutOrder> cutOrders;
        int                   nbCutX = static_cast<int>(std::ceil((bboxMax.x - bboxMin.x) / config.maxPieceSize.x)) - 1;
        int                   nbCutY = static_cast<int>(std::ceil((bboxMax.y - bboxMin.y) / config.maxPieceSize.y)) - 1;
        int                   nbCutZ = static_cast<int>(std::ceil((bboxMax.z - bboxMin.z) / config.maxPieceSize.z)) - 1;

        splinter3D::utils::Logger::getInstance().clog(
            "[MeshDissector] Cutting into grid of ", nbCutX + 1, " x ", nbCutY + 1, " x ", nbCutZ + 1,
            " = ", (nbCutX + 1) * (nbCutY + 1) * (nbCutZ + 1), " pieces\n");

        for (int i = 0; i < nbCutX; ++i)
        {
            Vec3 planePoint{
                bboxMin.x + (float) (i + 1) * config.maxPieceSize.x,
                bboxMin.y,
                bboxMin.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{1, 0, 0}, i});
        }
        for (int i = 0; i < nbCutY; ++i)
        {
            Vec3 planePoint{
                bboxMin.x,
                bboxMin.y + (float) (i + 1) * config.maxPieceSize.y,
                bboxMin.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{0, 1, 0}, nbCutX + i});
        }
        for (int i = 0; i < nbCutZ; ++i)
        {
            Vec3 planePoint{
                bboxMin.x,
                bboxMin.y,
                bboxMin.z + (float) (i + 1) * config.maxPieceSize.z};

            cutOrders.push_back({&mesh, planePoint, Vec3{0, 0, 1}, nbCutX + nbCutY + i});
        }

        return cutOrders;
    }
} // namespace geometry::utils::splitter