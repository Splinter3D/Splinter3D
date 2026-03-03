#include <Gui/Button.hpp>
#include <iostream>

namespace gui
{

    Button::Button(std::string          id,
                   ActionFn             action,
                   DrawIconFn           drawIcon,
                   renderer::IRenderer& renderer,
                   bool                 hasShortcut,
                   std::string          tooltip,
                   renderer::Key        shortcutKey,
                   bool                 hasPanel,
                   PanelDrawFn          panelDrawFn)
        : id_(std::move(id))
        , action_(std::move(action))
        , hasShortcut_(hasShortcut)
        , tooltip_(std::move(tooltip))
        , shortcutKey_(shortcutKey)
        , hasPanel_(hasPanel)
        , panelDrawFn_(std::move(panelDrawFn))
    {
        // Bake the icon once into a GPU texture
        if (drawIcon)
            iconTexture_ = renderer.createIcon(64, 64, [drawIcon](void* canvas) { drawIcon(canvas); });
    }

    // ── Update ────────────────────────────────────────────────────────────────────

    void Button::update(renderer::IRenderer& renderer)
    {
        const bool  hovered = isHovered(renderer);
        const float dt      = renderer.getDeltaTime();

        hoverTimer_ = hovered ? hoverTimer_ + dt : 0.0f;

        if (hovered && renderer.isMouseButtonPressed((int) renderer::MouseButton::Left))
        {
            if (hasPanel_)
                panelOpen_ = !panelOpen_;
            else
            {
                panelOpen_ = false;
                if (action_)
                    action_();
            }
        }

        if (hasShortcut_ && renderer.isKeyPressed(shortcutKey_))
        {
            if (hasPanel_)
                panelOpen_ = !panelOpen_;
            else if (action_)
                action_();
        }
    }

    // ── Draw ──────────────────────────────────────────────────────────────────────

    void Button::draw(const renderer::IRenderer& renderer) const
    {
        (void) renderer;
    }

    void Button::drawBase(const renderer::IRenderer& renderer) const
    {
        if (panelOpen_)
            renderer.drawRectangle(x, y, width, height, renderer::Palette::Accent, renderer::Layer::Overlay);
        else
            renderer.drawRectangle(x, y, width, height, renderer::Palette::Primary, renderer::Layer::UI);

        renderer.drawRectangleLines(x, y, width, height, renderer::Palette::Muted, renderer::Layer::UI);

        if (iconTexture_)
        {
            constexpr float kIconPadding = 0.55f;
            const float     iconSize     = std::min(width, height) * kIconPadding;
            const float     ix           = x + (width - iconSize) * 0.5f;
            const float     iy           = y + (height - iconSize) * 0.5f;
            renderer.drawTexture(iconTexture_, ix, iy, iconSize, iconSize, renderer::Layer::UI);
        }
    }

    void Button::drawOverlay(const renderer::IRenderer& renderer) const
    {
        const bool hovered = isHovered(renderer);

        // Hover tint — on top of ALL buttons
        if (hovered && !panelOpen_)
            renderer.drawRectangle(x, y, width, height,
                                   renderer::Color(renderer::Palette::Secondary.r,
                                                   renderer::Palette::Secondary.g,
                                                   renderer::Palette::Secondary.b,
                                                   80));

        // Tooltip — on top of ALL buttons
        if (hovered && hoverTimer_ >= kTooltipDelay)
            drawTooltip(renderer);

        // Panel — on top of ALL buttons
        if (panelOpen_)
            drawPanel(renderer);
    }

    // ── Private ───────────────────────────────────────────────────────────────────

    bool Button::isHovered(const renderer::IRenderer& renderer) const
    {
        const float mx = renderer.getMouseX();
        const float my = renderer.getMouseY();
        return mx >= x && mx <= x + width && my >= y && my <= y + height;
    }

    void Button::drawTooltip(const renderer::IRenderer& renderer) const
    {
        if (tooltip_.empty())
            return;

        constexpr float kPad = 6.0f;
        constexpr float kH   = 24.0f;
        const float     tw   = renderer.measureTextWidth(tooltip_.c_str(), 14) + kPad * 2.0f;

        float tx = renderer.getMouseX() + 20.0f;
        float ty = renderer.getMouseY() + 20.0f;

        renderer.drawRectangle(tx, ty, tw, kH, renderer::Palette::Background, renderer::Layer::UI);
        renderer.drawRectangleLines(tx, ty, tw, kH, renderer::Palette::Muted, renderer::Layer::UI);
        renderer.drawText(tooltip_.c_str(), tx + kPad, ty + (kH - 14.0f) * 0.5f, 14, renderer::Layer::UI);
    }

    void Button::drawPanel(const renderer::IRenderer& renderer) const
    {
        if (!panelDrawFn_)
            return;

        float px       = x + (width - kPanelW) * 0.5f;
        px             = std::max(4.0f, std::min(px, (float) renderer.getScreenWidth() - kPanelW - 4.0f));
        const float py = y + height + kPanelGap;

        // Shadow
        renderer.drawRectangle(px + 4.0f, py + 4.0f, kPanelW, kPanelH, renderer::Palette::Muted, renderer::Layer::UI);

        // Background + border
        renderer.drawRectangle(px, py, kPanelW, kPanelH, renderer::Palette::Background, renderer::Layer::UI);
        renderer.drawRectangleLines(px, py, kPanelW, kPanelH, renderer::Palette::Secondary, renderer::Layer::UI);

        panelDrawFn_(renderer, px, py, kPanelW, kPanelH);
    }

} // namespace gui