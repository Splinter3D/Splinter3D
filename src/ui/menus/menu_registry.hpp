#pragma once

#include "ui/framework/wx/registry/object_registry.hpp"

#include <wx/menu.h>

namespace ui::menus
{
    /**
     * @brief Registry responsible for creating application menus on demand.
     *
     * Maps a menu identifier (defined in
     * ui::framework::wx::ids::menus) to the factory responsible for
     * constructing the corresponding @c wxMenu.
     *
     * Unlike panels or other @c wxWindow-derived widgets, menus are created
     * without a parent window. Consequently this registry is based on
     * ObjectRegistry rather than WidgetRegistry.
     *
     * The registry is populated once during application bootstrap through
     * RegisterAllMenus(), then queried by windows when building their
     * menu bar.
     *
     * Example:
     * @code{.cpp}
     * auto* fileMenu =
     *     MenuRegistry::getInstance().Create(
     *         ui::framework::wx::ids::menus::kFile);
     * @endcode
     */
    using MenuRegistry = ui::framework::wx::registry::ObjectRegistry<wxMenu>;

} // namespace ui::menus
