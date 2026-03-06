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

        /**
         * Initializes the toolbar, creating its buttons. Should be called once before the first draw().
         */
        void initialize(renderer::IRenderer& renderer) override;

        /**
         * Draws the toolbar and its buttons. Should be called every frame after update().
         */
        void update(renderer::IRenderer& renderer);

        /**
         * Draws the toolbar and its buttons. Should be called every frame after update().
         */
        void draw(const renderer::IRenderer& renderer) const override;

      private:
        /**
         * Checks if the screen size has changed since the last frame, and if so, recalculates button positions to keep the toolbar centered.
         */
        void rebuildIfResized(renderer::IRenderer& renderer);

        /**
         * Recalculates button positions to keep the toolbar centered based on the current screen width and button layout.
         */
        void repositionButtons(renderer::IRenderer& renderer);

        float topMargin_;
        float buttonSize_;
        float spacing_;

        std::vector<Button> buttons_;
    };

} // namespace gui
