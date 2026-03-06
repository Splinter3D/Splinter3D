// RotationPanelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>

namespace gui::states
{
    class RotationPanelState : public splinter3D::utils::Singleton<RotationPanelState>
    {
      public:
        // Rotation values for the selected object
        float rotX{0.0f};
        float rotY{0.0f};
        float rotZ{0.0f};
        // Input rotation values (Used to sync the rotation values after editing)
        float rotXInput{0.0f};
        float rotYInput{0.0f};
        float rotZInput{0.0f};
        // Whether the user is currently editing the X/Y/Z value (used to prevent overwriting the value while editing)
        bool editX{false};
        bool editY{false};
        bool editZ{false};

        /**
         * Applies the current rotation values to the selected object in the scene.
         */
        void applyToTarget()
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

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset rotation values when the selection changes.
         */
        RotationPanelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<scene::events::ObjectSelectedEvent>(
                    [this](const scene::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~RotationPanelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the rotation values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current rotation and updates the rotation values accordingly.
         * If no object is selected (index -1), it resets the rotation values to 0 for all axes.
         */
        void resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
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

        friend class Singleton<RotationPanelState>;
    };
} // namespace gui::states
