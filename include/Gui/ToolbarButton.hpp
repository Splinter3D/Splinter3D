#pragma once
#include <Gui/IGuiComponent.hpp>
#include <functional>
#include <string>

namespace renderer
{
    struct ITexture;
    class IRenderer;
} // namespace renderer

namespace gui
{
    struct ToolbarButton : IGuiComponent
    {
        std::string           id;
        std::function<void()> onClick;
        renderer::ITexture*   icon{nullptr};
        bool                  hasIcon{false};

        float x{0};
        float y{0};
        float width{48};
        float height{48};

        ToolbarButton(
            const std::string&                _id,
            std::function<void()>             _onClick,
            const std::function<void(void*)>& iconPainter,
            renderer::IRenderer&              renderer,
            bool                              _hasIcon = true);

        void draw(const renderer::IRenderer& renderer) const override;
    };
} // namespace gui