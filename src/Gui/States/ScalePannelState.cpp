#include <Gui/States/ScalePannelState.hpp>

namespace gui::states
{

    void ScalePannelState::applyToTarget()
    {
        scene::SceneObject* target = scene::Scene::getInstance().getSelected();
        if (!target)
            return;
        objects3D::Transform t = target->getTransform();
        t.scale.x              = scaleX / 100.0f;
        t.scale.y              = scaleY / 100.0f;
        t.scale.z              = scaleZ / 100.0f;
        target->setTransform(t);
    }

    void ScalePannelState::resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
    {
        int index = e.index;
        if (index < 0)
        {
            scaleX = scaleY = scaleZ = 100.0f;
            scaleXInput = scaleYInput = scaleZInput = 100.0f;
        }
        else
        {
            auto* obj = scene::Scene::getInstance().getSelected();
            if (!obj)
                return;
            const auto t = obj->getTransform();
            scaleX       = t.scale.x * 100.0f;
            scaleY       = t.scale.y * 100.0f;
            scaleZ       = t.scale.z * 100.0f;
            scaleXInput  = scaleX;
            scaleYInput  = scaleY;
            scaleZInput  = scaleZ;
        }
    }

} // namespace gui::states
