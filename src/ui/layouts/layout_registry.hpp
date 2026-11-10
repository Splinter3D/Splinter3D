#pragma once

#include "base_layout/base_layout.hpp"
#include "ui/framework/wx/registry/widget_registry.hpp"

namespace ui::layouts
{
    using LayoutRegistry = ui::framework::wx::registry::WidgetRegistry<BaseLayout>;
} // namespace ui::layouts
