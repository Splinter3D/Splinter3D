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

        /**
         * Applies the current BedSize values to the selected object in the scene.
         */
        void applyToTarget();

      protected:
        ~CutPannelState() noexcept = default;

      private:
        friend class Singleton<CutPannelState>;
    };
} // namespace gui::states
