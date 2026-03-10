#include <Geometry/Utils/MeshUtils.hpp>
#include <Geometry/Vec3.hpp>
#include <Gui/States/CutPannelState.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>

namespace gui::states
{
    void CutPannelState::executeCut()
    {
        scene::Scene::getInstance().splitSelected(bedSizeX, bedSizeY, bedSizeZ);
    }

    void CutPannelState::resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e)
    {
        if (e.index >= 0)
        {
            if (e.index == lastSelectedId)
                return; // No change in selection, do nothing
            lastSelectedId = e.index;
            auto* selected = scene::Scene::getInstance().getSelected();
            if (selected)
            {
                geometry::utils::MeshBounds bounds = geometry::utils::computeMeshBounds(*selected->getTransformedMesh());
                bedSizeX                           = bounds.size.x;
                bedSizeY                           = bounds.size.y;
                bedSizeZ                           = bounds.size.z;
                bedSizeXInput                      = bounds.size.x;
                bedSizeYInput                      = bounds.size.y;
                bedSizeZInput                      = bounds.size.z;
                std::cout << "Selected object bounds: " << bedSizeX << " x " << bedSizeY << " x " << bedSizeZ << std::endl;
            }
        }
        else
        {
            bedSizeX      = 0.0f;
            bedSizeY      = 0.0f;
            bedSizeZ      = 0.0f;
            bedSizeXInput = 0.0f;
            bedSizeYInput = 0.0f;
            bedSizeZInput = 0.0f;
        }
    }
} // namespace gui::states
