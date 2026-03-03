#include <Gui/ToolbarButton.hpp>

namespace gui
{
    ToolbarButton::ToolbarButton(const std::string& _id, std::function<void()> _onClick, const std::function<void(Image&)>& iconPainter)
    {
        id      = _id;
        onClick = std::move(_onClick);
        if (iconPainter)
        {
            icon    = createToolbarIcon(iconPainter);
            hasIcon = true;
        }
    }

    Texture2D ToolbarButton::createToolbarIcon(const std::function<void(Image&)>& painter)
    {
        constexpr int iconSize = 64;
        Image         canvas   = GenImageColor(iconSize, iconSize, Color{0, 0, 0, 0});
        painter(canvas);
        Texture2D texture = LoadTextureFromImage(canvas);
        UnloadImage(canvas);
        return texture;
    }
} // namespace gui
