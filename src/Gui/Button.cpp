#include <Gui/Button.hpp>
#include <Renderer/Palette.hpp>

namespace gui
{

    Button::Button(std::string                id,
                   ActionFn                   action,
                   DrawIconFn                 drawIcon,
                   renderer::IRenderer&       renderer,
                   bool                       hasShortcut,
                   std::string                tooltip,
                   std::vector<renderer::Key> shortcutKey,
                   bool                       hasPanel,
                   PanelDrawFn                panelDrawFn)
        : id_(std::move(id))
        , action_(std::move(action))
        , hasShortcut_(hasShortcut)
        , tooltip_(std::move(tooltip))
        , shortcutKeys_(shortcutKey)
        , hasPanel_(hasPanel)
        , panelDrawFn_(std::move(panelDrawFn))
    {
        // Bake the icon once into a GPU texture
        if (drawIcon)
            iconTexture_ = renderer.createIcon(64, 64, [drawIcon](void* canvas) { drawIcon(canvas); });
    }

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

        if (hasShortcut_)
        {
            bool modifiersHeld      = true;
            bool hasNonModifierKey  = false;
            bool nonModifierPressed = false;

            for (const auto& key : shortcutKeys_)
            {
                const bool isModifier = (key == renderer::Key::Ctrl ||
                                         key == renderer::Key::Shift ||
                                         key == renderer::Key::Alt);

                if (isModifier)
                {
                    if (!renderer.isKeyDown(key))
                    {
                        modifiersHeld = false;
                        break;
                    }
                }
                else
                {
                    hasNonModifierKey = true;
                    if (renderer.isKeyPressed(key))
                        nonModifierPressed = true;
                }
            }

            const bool shortcutTriggered = modifiersHeld && hasNonModifierKey && nonModifierPressed;
            if (shortcutTriggered)
            {
                if (hasPanel_)
                    panelOpen_ = !panelOpen_;
                else if (action_)
                    action_();
            }
        }
    }

    void Button::draw(const renderer::IRenderer& renderer) const
    {
        // Base — UI layer
        if (panelOpen_)
            renderer.drawRectangle(x, y, width, height, renderer::Palette::Secondary, renderer::Layer::UI);
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

        // Overlay layer — hover tint, tooltip, panel
        const bool hovered = isHovered(renderer);

        if (hovered && !panelOpen_)
            renderer.drawRectangle(x, y, width, height,
                                   renderer::Color(renderer::Palette::Secondary.r,
                                                   renderer::Palette::Secondary.g,
                                                   renderer::Palette::Secondary.b,
                                                   80),
                                   renderer::Layer::Overlay);

        if (hovered && hoverTimer_ >= kTooltipDelay)
            drawTooltip(renderer);

        if (panelOpen_)
            drawPanel(renderer);
    }

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

        renderer.drawRectangle(tx, ty, tw, kH, renderer::Palette::Background, renderer::Layer::Overlay);
        renderer.drawRectangleLines(tx, ty, tw, kH, renderer::Palette::Muted, renderer::Layer::Overlay);
        renderer.drawText(tooltip_.c_str(), tx + kPad, ty + (kH - 14.0f) * 0.5f, 14, renderer::Layer::Overlay);
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