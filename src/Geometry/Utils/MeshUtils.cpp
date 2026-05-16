#include <Geometry/Utils/MeshUtils.hpp>

namespace geometry::utils
{
    void frameCameraOnMesh(renderer::IRenderer& renderer, const BBox& bounds)
    {
        if (!bounds.isValid())
            return;

        geometry::Vec3 center = bounds.getCenter();
        renderer.setCameraTarget(center);

        float radius   = std::max(bounds.getRadius(), 0.5f);
        float distance = radius * 2.0f;

        renderer.setOrbitDistance(distance);
        renderer.setOrbitAngles(0.8f, 0.6f);
    }
} // namespace geometry::utils
