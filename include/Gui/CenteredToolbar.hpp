#pragma once
#include <Gui/AGuiComponent.hpp>
#include <Gui/Button.hpp>
#include <vector>

namespace gui
{

    class CenteredToolbar : public AGuiComponent
    {
      public:
        CenteredToolbar(float topMargin  = 16.0f,
                        float buttonSize = 48.0f,
                        float spacing    = 12.0f);
        ~CenteredToolbar() = default;

        void initialize(renderer::IRenderer& renderer);

        // Call every frame BEFORE draw()
        void update(renderer::IRenderer& renderer);

        void draw(const renderer::IRenderer& renderer) const override;

      private:
        void rebuildIfResized(renderer::IRenderer& renderer);
        void repositionButtons(renderer::IRenderer& renderer);

        float topMargin_;
        float buttonSize_;
        float spacing_;

        std::vector<Button> buttons_;
    };

} // namespace gui