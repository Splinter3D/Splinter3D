#include <Gui/Button.hpp>
#include <Renderer/Palette.hpp>
#include <Splinter3D/Events/EventBus.hpp>
#include <Splinter3D/Events/OpenPannelEvent.hpp>

namespace gui
{

    Button::Button(std::string          id,
                   ActionFn             action,
                   DrawIconFn           drawIcon,
                   renderer::IRenderer& renderer,
                   std::string          tooltip,
                   bool                 hasPannel,
                   PannelDrawFn         pannelDrawFn)
        : id_(std::move(id))
        , action_(std::move(action))
        , tooltip_(std::move(tooltip))
        , hasPannel_(hasPannel)
        , pannelDrawFn_(std::move(pannelDrawFn))
    {
        // Bake the icon once into a GPU texture
        if (drawIcon)
            iconTexture_ = renderer.createIcon(64, 64, [drawIcon](void* canvas) { drawIcon(canvas); });
    }

    void Button::subscribeToPannelEvents()
    {
        if (hasPannel_)
        {
            splinter3D::events::EventBus::getInstance().subscribe<splinter3D::events::OpenPannelEvent>(
                [this](const splinter3D::events::OpenPannelEvent& e) { reactToOpenPannelEvent(e); });
        }
    }

    void Button::reactToOpenPannelEvent(const splinter3D::events::OpenPannelEvent& e)
    {
        if (e.buttonId == id_ && hasPannel_)
            pannelOpen_ = true;
        else
            pannelOpen_ = false;
    }

    void Button::update(renderer::IRenderer& renderer)
    {
        const bool  hovered = isHovered(renderer);
        const float dt      = renderer.getDeltaTime();

        hoverTimer_ = hovered ? hoverTimer_ + dt : 0.0f;

        if (hovered && renderer.isMouseButtonPressed((int) renderer::MouseButton::Left))
        {
            if (hasPannel_)
                pannelOpen_ = !pannelOpen_;
            else
            {
                pannelOpen_ = false;
                if (action_)
                    action_();
            }
        }
    }

    void Button::draw(const renderer::IRenderer& renderer) const
    {
        // Base — UI layer
        if (pannelOpen_)
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
            renderer.drawTexture(ix, iy, iconSize, iconSize, iconTexture_, renderer::Layer::UI);
        }

        // Overlay layer — hover tint, tooltip, pannel
        const bool hovered = isHovered(renderer);

        if (hovered && !pannelOpen_)
            renderer.drawRectangle(x, y, width, height,
                                   renderer::Color(renderer::Palette::Secondary.r,
                                                   renderer::Palette::Secondary.g,
                                                   renderer::Palette::Secondary.b,
                                                   80),
                                   renderer::Layer::Overlay);

        if (hovered && hoverTimer_ >= kTooltipDelay)
            drawTooltip(renderer);

        if (pannelOpen_)
            drawPannel(renderer);
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
        renderer.drawText(tx + kPad, ty + (kH - 14.0f) * 0.5f, tooltip_.c_str(), 14, renderer::Layer::Overlay);
    }

    void Button::drawPannel(const renderer::IRenderer& renderer) const
    {
        if (!pannelDrawFn_)
            return;

        float px       = x + (width - kPannelW) * 0.5f;
        px             = std::max(4.0f, std::min(px, (float) renderer.getScreenWidth() - kPannelW - 4.0f));
        const float py = y + height + kPannelGap;

        const float measuredHeight = pannelDrawFn_(renderer, px, py, kPannelW);
        const float pannelHeight   = measuredHeight > 0.0f ? measuredHeight : kPannelH;
        pannelHeight_              = pannelHeight;

        // Shadow
        renderer.drawRectangle(px + 4.0f, py + 4.0f, kPannelW, pannelHeight, renderer::Palette::Muted, renderer::Layer::UI);

        // Background + border
        renderer.drawRectangle(px, py, kPannelW, pannelHeight, renderer::Palette::Background, renderer::Layer::UI);
        renderer.drawRectangleLines(px, py, kPannelW, pannelHeight, renderer::Palette::Secondary, renderer::Layer::UI);
    }

} // namespace gui
