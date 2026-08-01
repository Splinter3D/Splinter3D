// RotationPannel.hpp
#pragma once
#include <Gui/States/RotationPannelState.hpp>
#include <Renderer/IRenderer.hpp>

namespace gui::pannels
{
    struct RotationPannel
    {
        /**
         * Draws the move panel UI using the provided renderer.
         */
        float operator()(const renderer::IRenderer& r,
                         float px, float py,
                         float pw) const;
    };
} // namespace gui::pannels
