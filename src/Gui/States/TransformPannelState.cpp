#include <Gui/States/TransformPannelState.hpp>

namespace gui::states
{

    void TransformPannelState::applyToTarget()
    {
        scene::SceneObject* target = scene::Scene::getInstance().getSelected();
        if (!target)
            return;
        objects3D::Transform t = target->getTransform();
        t.position.x           = posX;
        t.position.y           = posY;
        t.position.z           = posZ;
        target->setTransform(t);
    }

    void TransformPannelState::resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
    {
        int index = e.index;
        if (index < 0)
        {
            posX = posY = posZ = 0.0f;
            posXi = posYi = posZi = 0;
        }
        else
        {
            auto* obj = scene::Scene::getInstance().getSelected();
            if (!obj)
                return;
            const auto t = obj->getTransform();
            posX         = t.position.x;
            posY         = t.position.y;
            posZ         = t.position.z;
            posXi        = (int) posX;
            posYi        = (int) posY;
            posZi        = (int) posZ;
        }
    }

} // namespace gui::states
