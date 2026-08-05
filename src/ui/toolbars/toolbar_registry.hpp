#pragma once

#include "ui/framework/wx/registry/widget_registry.hpp"

#include <wx/toolbar.h>

namespace ui::toolbars
{
    /**
     * @brief Registry responsible for creating application toolbars on demand.
     *
     * Maps a toolbar identifier (defined in
     * ui::framework::wx::ids::toolbars) to the factory responsible for
     * constructing the corresponding @c wxToolBar.
     *
     * Unlike wxFrame::CreateToolBar(), which only supports a single native
     * toolbar per frame, this registry creates regular parented
     * @c wxToolBar instances. They can therefore be freely inserted into a
     * window layout, stacked, hidden, or later managed by wxAUI for docking.
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
        ui::framework::wx::registry::WidgetRegistry<wxToolBar>;

} // namespace ui::toolbars
