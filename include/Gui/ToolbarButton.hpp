#pragma once

#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

namespace gui
{
    struct ToolbarButton
    {
        std::string           id;
        Texture2D             icon{};
        bool                  hasIcon{false};
        std::function<void()> onClick;

        ToolbarButton(const std::string& _id, std::function<void()> _onClick, const std::function<void(Image&)>& iconPainter);

        Texture2D createToolbarIcon(const std::function<void(Image&)>& painter);
    };

} // namespace gui