#pragma once

#include "ui/framework/wx/registry/widget_registry.hpp"

#include <wx/panel.h>

namespace ui::panels
{
    /**
     * @brief Registry of application panels.
     *
     * The PanelRegistry associates a panel identifier with the factory
     * responsible for constructing the corresponding wxPanel.
     *
     * It is populated once during application bootstrap by
     * RegisterAllPanels() and can then be used by any window or layout
     * needing to instantiate a panel dynamically.
     *
     * Example:
     *
     * @code{.cpp}
     * auto* panel =
     *     PanelRegistry::getInstance().Create(
     *         ui::framework::wx::ids::panels::kExample,
     *         parent);
     * @endcode
     *
     * Adding a new panel only requires registering its factory inside
     * RegisterAllPanels(); existing windows do not need to be modified.
     */
    using PanelRegistry = ui::framework::wx::registry::WidgetRegistry<wxPanel>;

} // namespace ui::panels
