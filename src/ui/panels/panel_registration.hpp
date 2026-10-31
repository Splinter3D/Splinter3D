#pragma once

namespace ui::panels
{
    /**
     * @brief Registers every application panel.
     *
     * This function populates the global PanelRegistry with the factory
     * associated with each available panel.
     *
     * It must be called exactly once during application startup, typically
     * from the bootstrap phase, before any window attempts to create panels.
     *
     * When adding a new panel to the application, simply register its factory
     * inside the implementation of this function.
     */
    void RegisterAllPanels();

} // namespace ui::panels
