#pragma once

#include <Gui/CenteredToolbar.hpp>
#include <Renderer/IRenderer.hpp>

namespace input
{
    void registerBindings(renderer::IRenderer& renderer, gui::CenteredToolbar& toolbar);
} // namespace input
