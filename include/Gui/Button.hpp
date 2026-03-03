#pragma once
#include <Gui/AGuiComponent.hpp>
#include <Renderer/IRenderer.hpp>
#include <functional>
#include <string>

namespace gui
{

    class Button : public AGuiComponent
    {
      public:
        using DrawIconFn  = std::function<void(void* canvas)>;
        using ActionFn    = std::function<void()>;
        using PanelDrawFn = std::function<void(
            const renderer::IRenderer&,
            float panelX, float panelY,
            float panelW, float panelH)>;

        // Set by CenteredToolbar after construction
        float x{0}, y{0}, width{48}, height{48};

        Button(std::string          id,
               ActionFn             action,
               DrawIconFn           drawIcon,
               renderer::IRenderer& renderer,
               bool                 hasShortcut = false,
               std::string          tooltip     = "",
               renderer::Key        shortcutKey = renderer::Key::Unknown,
               bool                 hasPanel    = false,
               PanelDrawFn          panelDrawFn = nullptr);

        ~Button() = default;

        // Call once per frame — handles click + keyboard shortcut
        void update(renderer::IRenderer& renderer);

        // AGuiComponent
        void draw(const renderer::IRenderer& renderer) const override;
        void drawBase(const renderer::IRenderer& renderer) const;
        void drawOverlay(const renderer::IRenderer& renderer) const;

        void initialize(renderer::IRenderer& renderer) override
        {
            (void) renderer;
        }

        // Panel state
        bool isPanelOpen() const
        {
            return panelOpen_;
        }

        void closePanel()
        {
            panelOpen_ = false;
        }

      private:
        bool isHovered(const renderer::IRenderer& renderer) const;
        void drawTooltip(const renderer::IRenderer& renderer) const;
        void drawPanel(const renderer::IRenderer& renderer) const;

        std::string         id_;
        ActionFn            action_;
        renderer::ITexture* iconTexture_ = nullptr;
        bool                hasShortcut_;
        std::string         tooltip_;
        renderer::Key       shortcutKey_;
        bool                hasPanel_;
        PanelDrawFn         panelDrawFn_;

        mutable bool panelOpen_{false};

        static constexpr float kPanelW       = 220.0f;
        static constexpr float kPanelH       = 150.0f;
        static constexpr float kPanelGap     = 6.0f; // gap below button
        static constexpr float kTooltipDelay = 0.2f; // seconds before tooltip shows
        mutable float          hoverTimer_{0.0f};
    };

} // namespace gui