#pragma once

#include <Gui/ToolbarButton.hpp>
#include <Renderer/IRenderer.hpp>
#include <Renderer/RayGUI.hpp>
#include <functional>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

namespace renderer
{
    class IRenderer; // forward declaration
} // namespace renderer

namespace gui
{
    class IGuiComponent
    {
      public:
        virtual void draw(const renderer::IRenderer& renderer) const = 0;
    };
} // namespace gui