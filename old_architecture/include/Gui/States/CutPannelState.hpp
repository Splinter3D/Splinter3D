// CutPannelState.hpp
#pragma once
#include <Objects3D/Object3D.hpp>
#include <Scene/Scene.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/ObjectSelectedEvent.hpp>
#include <Splinter3D/Utils/Singleton.hpp>

namespace gui::states
{
    class CutPannelState : public splinter3D::utils::Singleton<CutPannelState>
    {
      public:
        // Current BedSize values
        float bedSizeX{0.0f};
        float bedSizeY{0.0f};
        float bedSizeZ{0.0f};
        // Input BedSize values (Used to sync the BedSize values after editing)
        float bedSizeXInput{0.0f};
        float bedSizeYInput{0.0f};
        float bedSizeZInput{0.0f};
        // Whether the user is currently editing the X/Y/Z value (used to prevent overwriting the value while editing)
        bool editX{false};
        bool editY{false};
        bool editZ{false};
        // Last id of the selected object (used to detect selection changes and reset the bed size values accordingly)
        int lastSelectedId{-1};

        /**
         * Applies the current cut settings to the selected target object in the scene.
         */
        void executeCut();

      protected:
        /**
         * Constructor that subscribes to ObjectSelectedEvent to reset the base size to match the model size values when the selection changes.
         */
        CutPannelState() noexcept
        {
            splinter3D::events::EventBus::getInstance()
                .subscribe<splinter3D::events::ObjectSelectedEvent>(
                    [this](const splinter3D::events::ObjectSelectedEvent& e) { resetOnSelectionChange(e); });
        }

        ~CutPannelState() noexcept = default;

      private:
        /**
         * Event handler for ObjectSelectedEvent. Resets the object size values based on the selected object.
         * If an object is selected (index >= 0), it retrieves the object's current size and updates the bed size values accordingly.
         * If no object is selected (index -1), it resets the bed size values to 0 for all axes.
         */
        void resetOnSelectionChange(const splinter3D::events::ObjectSelectedEvent& e);

        friend class Singleton<CutPannelState>;
    };
} // namespace gui::states
