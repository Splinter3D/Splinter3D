#pragma once

#include <Gui/IGuiComponent.hpp>
#include <Gui/ToolbarButton.hpp>
#include <Renderer/IRenderer.hpp>
#include <Renderer/RayGUI.hpp>
#include <functional>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

namespace gui
{
    class CenteredToolbar : public IGuiComponent
    {
      public:
        CenteredToolbar(float topMargin = 16.0f, float buttonSize = 48.0f, float spacing = 12.0f);

        ~CenteredToolbar();

        void initialize();

        void draw(const renderer::IRenderer& renderer) const override;

      private:
        float topMargin_{16.0f};
        float buttonSize_{48.0f};
        float spacing_{12.0f};

        std::vector<ToolbarButton> _toolbarButtons;

        static void drawIcon(const Rectangle& bounds, const Texture2D& icon);
    };
} // namespace gui