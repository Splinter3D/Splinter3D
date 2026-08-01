#include <Gui/States/RotationPannelState.hpp>

namespace gui::states
{

    void RotationPannelState::applyToTarget()
    {
        scene::SceneObject* target = scene::Scene::getInstance().getSelected();
        if (!target)
            return;
        objects3D::Transform t = target->getTransform();
        t.rotation.x           = rotX;
        t.rotation.y           = rotY;
        t.rotation.z           = rotZ;
        target->setTransform(t);
    }

    void RotationPannelState::resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e)
    {
        int index = e.index;
        if (index < 0)
        {
            rotX = rotY = rotZ = 0.0f;
            rotXInput = rotYInput = rotZInput = 0;
        }
        else
        {
            auto* obj = scene::Scene::getInstance().getSelected();
            if (!obj)
                return;
            const auto t = obj->getTransform();
            rotX         = t.rotation.x;
            rotY         = t.rotation.y;
            rotZ         = t.rotation.z;
            rotXInput    = rotX;
            rotYInput    = rotY;
            rotZInput    = rotZ;
        }
    }

} // namespace gui::states
