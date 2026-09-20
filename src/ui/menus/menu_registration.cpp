#include "menu_registration.hpp"

#include "menu_object/menu_object.hpp"
#include "menu_registry.hpp"
#include "menu_settings/menu_settings.hpp"
#include "ui/framework/wx/ids/ids.hpp"

// Add one include + one Register(...) line per real menu as they get built.

namespace ui::menus
{
    void RegisterAllMenus()
    {
        // Retrieve the singleton registry responsible for menu factories.
        auto& registry = MenuRegistry::getInstance();

        // Register each menu with the registry. Each menu is identified by a unique id
        // Each menu is identified by a unique id and associated with a
        // factory capable of constructing a fresh wxMenu instance whenever
        // a window requests it.
        registry.Register(ui::framework::wx::ids::menus::kObject,
                          []() {
                              return new MenuObject();
                          });
        registry.Register(ui::framework::wx::ids::menus::kSettings,
                          []() {
                              return new MenuSettings();
                          });
    }

} // namespace ui::menus
