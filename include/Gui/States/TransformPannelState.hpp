// TransformPannelState.hpp
#pragma once
#include <Geometry/Vec3.hpp>
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>
#include <unordered_map>

namespace gui::states
{
    class TransformPannelState : public splinter3D::utils::Singleton<TransformPannelState>
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
        void applyToTarget();

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset position values when the selection changes.
         */
        TransformPannelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<splinter3D::events::ObjectSelectedEvent>(
                    [this](const splinter3D::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~TransformPannelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the position values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current position and updates the position values accordingly.
         * If no object is selected (index -1), it resets the position values to 0 for all axes.
         */
        void resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e);

        void           captureSelectionOffsets();
        geometry::Vec3 getOffsetFor(const scene::SceneObject* obj) const;

        std::unordered_map<const scene::SceneObject*, geometry::Vec3> translationOffsets_;

        friend class Singleton<TransformPannelState>;
    };
} // namespace gui::states
