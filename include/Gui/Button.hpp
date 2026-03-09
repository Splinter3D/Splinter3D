#pragma once
#include <Gui/AGuiComponent.hpp>
#include <Renderer/IRenderer.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>
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
        using PannelDrawFn = std::function<float(
            const renderer::IRenderer&,
            float pannelX, float pannelY,
            float pannelW)>;

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
             * Sets a tooltip to show when the button is hovered.
             */
            Builder& tooltip(std::string text)
            {
                tooltip_ = std::move(text);
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
                              std::move(tooltip_), hasPannel_, std::move(pannelDrawFn_));
            }

          private:
            std::string  id_;
            ActionFn     action_;
            DrawIconFn   drawIcon_;
            std::string  tooltip_;
            bool         hasPannel_{false};
            PannelDrawFn pannelDrawFn_;
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

        Button(std::string          id,
               ActionFn             action,
               DrawIconFn           drawIcon,
               renderer::IRenderer& renderer,
               std::string          tooltip      = "",
               bool                 hasPannel    = false,
               PannelDrawFn         pannelDrawFn = nullptr);

        virtual ~Button() = default;

        /**
         * Manually subscribe to pannel open events.
         */
        void subscribeToPannelEvents();

        /**
         * Open its own pannel when the venet match the button id
         */
        void reactToOpenPannelEvent(const splinter3D::events::OpenPannelEvent& e);

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
         * Returns the latest measured pannel height to help other components
         * determine its bounds when opened.
         */
        float getPannelHeight() const
        {
            return hasPannel_ ? pannelHeight_ : 0.0f;
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

        std::string         id_;
        ActionFn            action_;
        renderer::ITexture* iconTexture_ = nullptr;
        std::string         tooltip_;
        bool                hasPannel_;
        PannelDrawFn        pannelDrawFn_;

        mutable bool  pannelOpen_{false};
        mutable float pannelHeight_{kPannelH};

        static constexpr float kTooltipDelay = 0.2f; // seconds before tooltip shows
        mutable float          hoverTimer_{0.0f};
    };

} // namespace gui
