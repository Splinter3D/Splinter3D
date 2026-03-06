// ScalePannelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>

namespace gui::states
{
    class ScalePannelState : public splinter3D::utils::Singleton<ScalePannelState>
    {
      public:
        // Scale values
        float scaleX{100.0f};
        float scaleY{100.0f};
        float scaleZ{100.0f};
        // Input scale values (Used to sync the scale values after editing)
        float scaleXInput{100.0f};
        float scaleYInput{100.0f};
        float scaleZInput{100.0f};
        // Edit mode flags for each axis
        bool editX{false};
        bool editY{false};
        bool editZ{false};
        // Whether to maintain uniform scaling across all axes
        bool uniformScale{true};

        /**
         * Applies the current scale values to the selected object in the scene.
         */
        void applyToTarget();

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset scale values when the selection changes.
         */
        ScalePannelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<splinter3D::events::ObjectSelectedEvent>(
                    [this](const splinter3D::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~ScalePannelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the scale values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current scale and updates the scale values accordingly.
         * If no object is selected (index -1), it resets the scale values to 100% (1.0 scale) for all axes.
         */
        void resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e);

        friend class Singleton<ScalePannelState>;
    };
} // namespace gui::states
