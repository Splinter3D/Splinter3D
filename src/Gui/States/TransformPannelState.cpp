#include <Gui/States/TransformPannelState.hpp>

namespace gui::states
{

    void TransformPannelState::applyToTarget()
    {
        auto targets = scene::Scene::getInstance().getSelectedObjects();
        if (targets.empty())
            return;

        for (auto* target : targets)
        {
            if (!target)
                continue;
            const auto           offset = getOffsetFor(target);
            objects3D::Transform t      = target->getTransform();
            t.position.x                = posX + offset.x;
            t.position.y                = posY + offset.y;
            t.position.z                = posZ + offset.z;
            target->setTransform(t);
        }
        captureSelectionOffsets();
    }

    void TransformPannelState::resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e)
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
            posXi        = posX;
            posYi        = posY;
            posZi        = posZ;
        }
        captureSelectionOffsets();
    }

    void TransformPannelState::captureSelectionOffsets()
    {
        translationOffsets_.clear();

        auto* reference = scene::Scene::getInstance().getSelected();
        if (!reference)
            return;

        const auto refTransform = reference->getTransform();
        for (auto* obj : scene::Scene::getInstance().getSelectedObjects())
        {
            if (!obj)
                continue;
            const auto t             = obj->getTransform();
            translationOffsets_[obj] = geometry::Vec3(
                t.position.x - refTransform.position.x,
                t.position.y - refTransform.position.y,
                t.position.z - refTransform.position.z);
        }
    }

    geometry::Vec3 TransformPannelState::getOffsetFor(const scene::SceneObject* obj) const
    {
        if (!obj)
            return geometry::Vec3();

        auto it = translationOffsets_.find(obj);
        if (it == translationOffsets_.end())
            return geometry::Vec3();
        return it->second;
    }

} // namespace gui::states
