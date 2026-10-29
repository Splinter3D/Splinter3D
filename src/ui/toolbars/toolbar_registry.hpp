#pragma once

#include "ui/framework/wx/registry/widget_registry.hpp"

#include <wx/window.h>

namespace ui::toolbars
{
    /**
     * @brief Registry responsible for creating application toolbars on demand.
     *
     * Maps a toolbar identifier (defined in
     * ui::framework::wx::ids::toolbars) to the factory responsible for
     * constructing the corresponding widget.
     *
     * Toolbars are regular parented @c wxWindow instances (a native
     * @c wxToolBar, or a plain @c wxPanel for bars that need custom layout
     * and interactions a native toolbar cannot easily provide). They can
     * therefore be freely inserted into a window layout, stacked, hidden, or
     * later managed by wxAUI for docking.
     *
     * The registry is populated once during application bootstrap through
     * RegisterAllToolBars(), then queried by windows when building their
     * interface.
     *
     * Example:
     * @code{.cpp}
     * auto* toolbar =
     *     ToolBarRegistry::getInstance().Create(
     *         ui::framework::wx::ids::toolbars::kMain,
     *         this);
     * @endcode
     */
    using ToolBarRegistry =
        ui::framework::wx::registry::WidgetRegistry<wxWindow>;

} // namespace ui::toolbars
