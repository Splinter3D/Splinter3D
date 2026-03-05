#pragma once

namespace renderer
{
    class IRenderer; // forward declaration
} // namespace renderer

namespace gui
{
    class IGuiComponent
    {
      public:
        virtual void initialize(renderer::IRenderer& renderer)       = 0;
        virtual void draw(const renderer::IRenderer& renderer) const = 0;
    };
} // namespace gui
