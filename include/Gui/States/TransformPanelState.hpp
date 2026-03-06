// TransformPanelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>

namespace gui::states
{
    class TransformPanelState : public splinter3D::utils::Singleton<TransformPanelState>
    {
      public:
        // Position values
        float posX{0.0f};
        float posY{0.0f};
        float posZ{0.0f};
        // Input position values (Used to sync the position values after editing)
        float posXi{0.0f};
        float posYi{0.0f};
        float posZi{0.0f};
        // Whether the user is currently editing the X/Y/Z value (used to prevent overwriting the value while editing)
        bool editX{false};
        bool editY{false};
        bool editZ{false};

        /**
         * Applies the current position values to the selected object in the scene.
         */
        void applyToTarget()
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

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset position values when the selection changes.
         */
        TransformPanelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<scene::events::ObjectSelectedEvent>(
                    [this](const scene::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~TransformPanelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the position values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current position and updates the position values accordingly.
         * If no object is selected (index -1), it resets the position values to 0 for all axes.
         */
        void resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
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

        friend class Singleton<TransformPanelState>;
    };
} // namespace gui::states
