// ScalePanelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>

namespace gui::states
{
    class ScalePanelState
    {
      public:
        static ScalePanelState& instance()
        {
            static ScalePanelState state;
            return state;
        }

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

        int lastSelectedObjectIndex{-1};

        void applyToTarget()
        {
            scene::Scene instance = scene::Scene::getInstance();

            int                 currentSelectedObjectIndex = scene::Scene::getInstance().getSelectedIndex();
            scene::SceneObject* target                     = scene::Scene::getInstance().getSelected();
            if (!target)
                return;
            objects3D::Transform t = target->getTransform();
            t.scale.x              = scaleX / 100.0f;
            t.scale.y              = scaleY / 100.0f;
            t.scale.z              = scaleZ / 100.0f;
            target->setTransform(t);
        }

      private:
        ScalePanelState() = default;
    };
} // namespace gui::states