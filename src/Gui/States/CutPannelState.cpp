#include <Gui/States/CutPannelState.hpp>

namespace gui::states
{

    void CutPannelState::applyToTarget()
    {
        scene::SceneObject* target = scene::Scene::getInstance().getSelected();
        if (!target)
            return;
    }
} // namespace gui::states
