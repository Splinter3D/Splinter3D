// ScalePannel.hpp
#pragma once
#include <Gui/States/ScalePannelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::pannels
{
    struct ScalePannel
    {
        /**
         * Draws the scale panel UI using the provided renderer.
         */
        float operator()(const renderer::IRenderer& r,
                         float px, float py,
                         float pw) const;
    };
} // namespace gui::pannels
