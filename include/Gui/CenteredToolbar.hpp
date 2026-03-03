#pragma once
#include <Gui/IGuiComponent.hpp>
#include <Gui/ToolbarButton.hpp>
#include <vector>

namespace gui
{
    class CenteredToolbar : public IGuiComponent
    {
      public:
        CenteredToolbar(float topMargin = 16.0f, float buttonSize = 48.0f, float spacing = 12.0f);
        ~CenteredToolbar();

        void initialize(renderer::IRenderer& renderer);
        void draw(const renderer::IRenderer& renderer) const override;

      private:
        float topMargin_{16.0f};
        float buttonSize_{48.0f};
        float spacing_{12.0f};

        std::vector<ToolbarButton> _toolbarButtons;
    };
} // namespace gui