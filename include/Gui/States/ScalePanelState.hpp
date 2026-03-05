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
        ScalePanelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<scene::events::ObjectSelectedEvent>(
                    [this](const scene::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~ScalePanelState() noexcept = default;

      private:
        void resetOnSelectionChange(const scene::events::ObjectSelectedEvent& e)
        {
            int index = e.index;
            if (index < 0)
            {
                std::cout << "No object selected, resetting scale to defaults." << std::endl;
                // No object selected, reset to defaults
                scaleX = scaleY = scaleZ = 100.0f;
                scaleXi = scaleYi = scaleZi = 100;
            }
            else
            {
                std::cout << "Object selected with index: " << index << ", resetting scale to object's current scale." << std::endl;
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