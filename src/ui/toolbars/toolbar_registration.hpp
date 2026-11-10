#pragma once

namespace ui::toolbars
{
    /**
     * @brief Registers every application toolbar into the ToolBarRegistry.
     *
     * This function centralizes the registration of all toolbar factories.
     * It must be called exactly once during application bootstrap before
     * any window attempts to create toolbars.
     *
     * When adding a new toolbar, only this registration function needs to
     * be updated. Existing windows remain unchanged.
     */
    void RegisterAllToolBars();

} // namespace ui::toolbars
