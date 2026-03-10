#include <Geometry/Vec3.hpp>
#include <Gui/States/CutPannelState.hpp>

namespace gui::states
{
    void CutPannelState::executeCut()
    {
        scene::Scene::getInstance().splitSelected(bedSizeX, bedSizeY, bedSizeZ);
    }
} // namespace gui::states
