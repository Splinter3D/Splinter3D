#pragma once
#include <Gui/AGuiComponent.hpp>
#include <Renderer/IRenderer.hpp>
#include <functional>
#include <string>

namespace gui
{

    class Button : public AGuiComponent
    {

#pragma region Builder

      public:
        using DrawIconFn  = std::function<void(void* canvas)>;
        using ActionFn    = std::function<void()>;
        using PanelDrawFn = std::function<void(
            const renderer::IRenderer&,
            float panelX, float panelY,
            float panelW, float panelH)>;

        /**
         * Builder class for constructing Button instances with a fluent interface
         */
        class Builder
        {
          public:
            explicit Builder(std::string id) : id_(std::move(id))
            { }

            /**
             * Sets the action to perform when the button is clicked.
             */
            Builder& action(ActionFn fn)
            {
                action_ = std::move(fn);
                return *this;
            }

            /**
             * Sets the function to draw the button's icon.
             */
            Builder& icon(DrawIconFn fn)
            {
                drawIcon_ = std::move(fn);
                return *this;
            }

            /**
             * Defines a keyboard shortcut for the button.
             */
            Builder& shortcut(std::vector<renderer::Key> keys, std::string tooltip = "")
            {
                shortcutKeys_ = std::move(keys);
                hasShortcut_  = true;
                tooltip_      = std::move(tooltip);
                return *this;
            }

            /**
             * Defines a panel to open when the button is clicked, with a custom draw function.
             */
            Builder& panel(PanelDrawFn fn)
            {
                hasPanel_    = true;
                panelDrawFn_ = std::move(fn);
                return *this;
            }

            /**
             * Builds the Button instance using the provided renderer (needed for icon texture creation).
             */
            Button build(renderer::IRenderer& renderer)
            {
                return Button(std::move(id_), std::move(action_), std::move(drawIcon_), renderer,
                              hasShortcut_, std::move(tooltip_), shortcutKeys_, hasPanel_, std::move(panelDrawFn_));
            }

          private:
            std::string                id_;
            ActionFn                   action_;
            DrawIconFn                 drawIcon_;
            bool                       hasShortcut_{false};
            std::string                tooltip_;
            std::vector<renderer::Key> shortcutKeys_ = {};
            bool                       hasPanel_{false};
            PanelDrawFn                panelDrawFn_;
        };

#pragma endregion
#pragma region Button

        // Default values for position and size; will be set by parent container
        float x{0}, y{0}, width{48}, height{48};

        Button(std::string                id,
               ActionFn                   action,
               DrawIconFn                 drawIcon,
               renderer::IRenderer&       renderer,
               bool                       hasShortcut  = false,
               std::string                tooltip      = "",
               std::vector<renderer::Key> shortcutKeys = {},
               bool                       hasPanel     = false,
               PanelDrawFn                panelDrawFn  = nullptr);

        ~Button() = default;

        /**
         * Updates the button state, handling clicks and shortcuts. Should be called every frame before draw().
         */
        void update(renderer::IRenderer& renderer);

        /**
         * Draws the button. Should be called every frame after update().
         */
        void draw(const renderer::IRenderer& renderer) const override;

        /**
         * Buttons don't have any special initialization needs, but we must implement this method from IGuiComponent.
         */
        void initialize(renderer::IRenderer& renderer) override
        {
            (void) renderer;
        }

        /**
         * Returns whether the button's panel is currently open.
         */
        bool isPanelOpen() const
        {
            return panelOpen_;
        }

        /**
         * Closes the panel if it was open.
         */
        void closePanel()
        {
            panelOpen_ = false;
        }

#pragma endregion
#pragma region Button Private

      private:
        /**
         * return true if the mouse is currently hovering over the button
         */
        bool isHovered(const renderer::IRenderer& renderer) const;

        /**
         * Draws the tooltip if the button is hovered and the hover timer has exceeded the tooltip delay.
         */
        void drawTooltip(const renderer::IRenderer& renderer) const;

        /**
         * Draws the panel if there is one assigned to this button and the panel is currently open.
         */
        void drawPanel(const renderer::IRenderer& renderer) const;

        std::string                id_;
        ActionFn                   action_;
        renderer::ITexture*        iconTexture_ = nullptr;
        bool                       hasShortcut_;
        std::string                tooltip_;
        std::vector<renderer::Key> shortcutKeys_;
        bool                       hasPanel_;
        PanelDrawFn                panelDrawFn_;

        mutable bool panelOpen_{false};

        static constexpr float kPanelW       = 220.0f;
        static constexpr float kPanelH       = 150.0f;
        static constexpr float kPanelGap     = 6.0f; // gap below button
        static constexpr float kTooltipDelay = 0.2f; // seconds before tooltip shows
        mutable float          hoverTimer_{0.0f};
    };

} // namespace gui