// ScalePanelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>

namespace gui::states
{
    class ScalePanelState : public splinter3D::utils::Singleton<ScalePanelState>
    {
      public:
        // Scale values
        float scaleX{100.0f};
        float scaleY{100.0f};
        float scaleZ{100.0f};
        // Input scale values (Used to sync the scale values after editing)
        int scaleXi{100};
        int scaleYi{100};
        int scaleZi{100};
        // Edit mode flags for each axis
        bool editX{false};
        bool editY{false};
        bool editZ{false};
        // Whether to maintain uniform scaling across all axes
        bool uniformScale{true};

        /**
         * Applies the current scale values to the selected object in the scene.
         */
        void applyToTarget()
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

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset scale values when the selection changes.
         */
        ScalePanelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<scene::events::ObjectSelectedEvent>(
                    [this](const scene::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~ScalePanelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the scale values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current scale and updates the scale values accordingly.
         * If no object is selected (index -1), it resets the scale values to 100% (1.0 scale) for all axes.
         */
        void resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
        {
            int index = e.index;
            if (index < 0)
            {
                scaleX = scaleY = scaleZ = 100.0f;
                scaleXi = scaleYi = scaleZi = 100;
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
                scaleXi      = (int) scaleX;
                scaleYi      = (int) scaleY;
                scaleZi      = (int) scaleZ;
            }
        }

        friend class Singleton<ScalePanelState>;
    };
} // namespace gui::states
