#include <Gui/States/ScalePannelState.hpp>

namespace gui::states
{

    void ScalePannelState::applyToTarget()
    {
        auto targets = scene::Scene::getInstance().getSelectedObjects();
        if (targets.empty())
            return;

        const float sx = scaleX / 100.0f;
        const float sy = scaleY / 100.0f;
        const float sz = scaleZ / 100.0f;

        for (auto* target : targets)
        {
            if (!target)
                continue;
            const auto offset     = getOffsetFor(target);
            objects3D::Transform t = target->getTransform();
            t.scale.x              = sx + offset.x;
            t.scale.y              = sy + offset.y;
            t.scale.z              = sz + offset.z;
            target->setTransform(t);
        }
        captureSelectionOffsets();
    }

    void ScalePannelState::resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e)
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
        captureSelectionOffsets();
    }

    void ScalePannelState::captureSelectionOffsets()
    {
        scaleOffsets_.clear();

        auto* reference = scene::Scene::getInstance().getSelected();
        if (!reference)
            return;

        const auto refTransform = reference->getTransform();
        for (auto* obj : scene::Scene::getInstance().getSelectedObjects())
        {
            if (!obj)
                continue;
            const auto t = obj->getTransform();
            scaleOffsets_[obj] = geometry::Vec3(
                t.scale.x - refTransform.scale.x,
                t.scale.y - refTransform.scale.y,
                t.scale.z - refTransform.scale.z);
        }
    }

    geometry::Vec3 ScalePannelState::getOffsetFor(const scene::SceneObject* obj) const
    {
        if (!obj)
            return geometry::Vec3();

        auto it = scaleOffsets_.find(obj);
        if (it == scaleOffsets_.end())
            return geometry::Vec3();
        return it->second;
    }

} // namespace gui::states
