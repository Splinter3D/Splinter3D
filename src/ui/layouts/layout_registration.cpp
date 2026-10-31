#include "layout_registration.hpp"

#include "layout_registry.hpp"
#include "main_layout/main_layout.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/two_column_layout/two_column_layout.hpp"

namespace ui::layouts
{
    void RegisterAllLayouts()
    {
        auto& registry = LayoutRegistry::getInstance();

        registry.Register(
            ui::framework::wx::ids::layouts::kMain,
            [](wxWindow* parent) { return new MainLayout(parent); });

        registry.Register(ui::framework::wx::ids::layouts::kTwoColumn,
                          [](wxWindow* parent) { return new TwoColumnLayout(parent); });
    }
} // namespace ui::layouts
