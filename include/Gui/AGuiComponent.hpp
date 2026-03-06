#pragma once

#include <Gui/IGuiComponent.hpp>

namespace gui
{
    class AGuiComponent : public IGuiComponent
    {
        /**
         * Initializes the component. Should be called once before the first draw().
         */
        void initialize(renderer::IRenderer& renderer) override = 0;

        /**
         * Draws the component. Should be called every frame after update().
         */
        void draw(const renderer::IRenderer& renderer) const override = 0;

      protected:
        int _lastScreenWidth{0};
        int _lastScreenHeight{0};
    };
} // namespace gui
