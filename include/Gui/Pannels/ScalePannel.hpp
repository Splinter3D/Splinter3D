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
        void operator()(const renderer::IRenderer& r,
                        float px, float py,
                        float /*pw*/, float /*ph*/) const;
    };
} // namespace gui::pannels
