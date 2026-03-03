#include <Gui/ToolbarButton.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui
{
    ToolbarButton::ToolbarButton(
        const std::string&                _id,
        std::function<void()>             _onClick,
        const std::function<void(void*)>& iconPainter,
        renderer::IRenderer&              renderer,
        bool                              _hasIcon)
        : id(_id), onClick(std::move(_onClick)), hasIcon(_hasIcon)
    {
        if (hasIcon && iconPainter)
            icon = renderer.createIcon(64, 64, iconPainter);
    }

    void ToolbarButton::initialize(renderer::IRenderer& renderer)
    {
        (void) renderer;
        // No dynamic initialization needed here
    }

    void ToolbarButton::draw(const renderer::IRenderer& renderer) const
    {
        renderer.drawButton(x, y, width, height, icon, onClick);
    }
} // namespace gui