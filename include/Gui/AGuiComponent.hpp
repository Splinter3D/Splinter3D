#pragma once

#include <Gui/IGuiComponent.hpp>

namespace gui
{
    class AGuiComponent : public IGuiComponent
    {
        void draw(const renderer::IRenderer& renderer) const override = 0;
        void initialize(renderer::IRenderer& renderer) override       = 0;

      protected:
        int _lastScreenWidth{0};
        int _lastScreenHeight{0};
    };
} // namespace gui