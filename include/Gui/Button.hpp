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
        using DrawIconFn   = std::function<void(void* canvas)>;
        using ActionFn     = std::function<void()>;
        using PannelDrawFn = std::function<void(
            const renderer::IRenderer&,
            float pannelX, float pannelY,
            float pannelW, float pannelH)>;

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
             * Defines a pannel to open when the button is clicked, with a custom draw function.
             */
            Builder& pannel(PannelDrawFn fn)
            {
                hasPannel_    = true;
                pannelDrawFn_ = std::move(fn);
                return *this;
            }

            /**
             * Builds the Button instance using the provided renderer (needed for icon texture creation).
             */
            Button build(renderer::IRenderer& renderer)
            {
                return Button(std::move(id_), std::move(action_), std::move(drawIcon_), renderer,
                              hasShortcut_, std::move(tooltip_), shortcutKeys_, hasPannel_, std::move(pannelDrawFn_));
            }

          private:
            std::string                id_;
            ActionFn                   action_;
            DrawIconFn                 drawIcon_;
            bool                       hasShortcut_{false};
            std::string                tooltip_;
            std::vector<renderer::Key> shortcutKeys_ = {};
            bool                       hasPannel_{false};
            PannelDrawFn               pannelDrawFn_;
        };

#pragma endregion
#pragma region Button

        // Default values for position and size; will be set by parent container
        float x{0};
        float y{0};
        float width{48};
        float height{48};

        static constexpr float kPannelW   = 220.0f;
        static constexpr float kPannelH   = 200.0f;
        static constexpr float kPannelGap = 6.0f; // gap below button

        Button(std::string                id,
               ActionFn                   action,
               DrawIconFn                 drawIcon,
               renderer::IRenderer&       renderer,
               bool                       hasShortcut  = false,
               std::string                tooltip      = "",
               std::vector<renderer::Key> shortcutKeys = {},
               bool                       hasPannel    = false,
               PannelDrawFn               pannelDrawFn = nullptr);

        virtual ~Button() = default;

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
         * Returns whether the button's pannel is currently open.
         */
        bool isPannelOpen() const
        {
            return pannelOpen_;
        }

        /**
         * Closes the pannel if it was open.
         */
        void closePannel()
        {
            pannelOpen_ = false;
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
         * Draws the pannel if there is one assigned to this button and the pannel is currently open.
         */
        void drawPannel(const renderer::IRenderer& renderer) const;

        std::string                id_;
        ActionFn                   action_;
        renderer::ITexture*        iconTexture_ = nullptr;
        bool                       hasShortcut_;
        std::string                tooltip_;
        std::vector<renderer::Key> shortcutKeys_;
        bool                       hasPannel_;
        PannelDrawFn               pannelDrawFn_;

        mutable bool pannelOpen_{false};

        static constexpr float kTooltipDelay = 0.2f; // seconds before tooltip shows
        mutable float          hoverTimer_{0.0f};
    };

} // namespace gui
