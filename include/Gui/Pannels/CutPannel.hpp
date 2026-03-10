// CutPannel.hpp
#pragma once
#include <Gui/States/CutPannelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::pannels
{
    struct CutPannel
    {
        /**
         * Draws the cut panel UI using the provided renderer.
         */
        float operator()(const renderer::IRenderer& r,
                         float px, float py,
                         float /*pw*/) const;
    };
} // namespace gui::pannels
