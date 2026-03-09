#include <Gui/States/RotationPannelState.hpp>

namespace gui::states
{

    void RotationPannelState::applyToTarget()
    {
        auto targets = scene::Scene::getInstance().getSelectedObjects();
        if (targets.empty())
            return;

        for (auto* target : targets)
        {
            if (!target)
                continue;
            const auto offset     = getOffsetFor(target);
            objects3D::Transform t = target->getTransform();
            t.rotation.x           = rotX + offset.x;
            t.rotation.y           = rotY + offset.y;
            t.rotation.z           = rotZ + offset.z;
            target->setTransform(t);
        }
        captureSelectionOffsets();
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
        captureSelectionOffsets();
    }

    void RotationPannelState::captureSelectionOffsets()
    {
        rotationOffsets_.clear();

        auto* reference = scene::Scene::getInstance().getSelected();
        if (!reference)
            return;

        const auto refTransform = reference->getTransform();
        for (auto* obj : scene::Scene::getInstance().getSelectedObjects())
        {
            if (!obj)
                continue;
            const auto t = obj->getTransform();
            rotationOffsets_[obj] = geometry::Vec3(
                t.rotation.x - refTransform.rotation.x,
                t.rotation.y - refTransform.rotation.y,
                t.rotation.z - refTransform.rotation.z);
        }
    }

    geometry::Vec3 RotationPannelState::getOffsetFor(const scene::SceneObject* obj) const
    {
        if (!obj)
            return geometry::Vec3();
        auto it = rotationOffsets_.find(obj);
        if (it == rotationOffsets_.end())
            return geometry::Vec3();
        return it->second;
    }

} // namespace gui::states
