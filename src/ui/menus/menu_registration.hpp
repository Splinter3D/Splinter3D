#pragma once

namespace ui::menus
{
    /**
     * @brief Registers every application menu into the MenuRegistry.
     *
     * This function centralizes the registration of all menu factories.
     * It must be called exactly once during application bootstrap before
     * any window attempts to build a menu bar.
     *
     * When adding a new menu, only this registration function needs to be
     * updated. Existing windows remain unchanged.
     */
    void RegisterAllMenus();

} // namespace ui::menus
