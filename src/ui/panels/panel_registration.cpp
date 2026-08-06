#include "panel_registration.hpp"

#include "example_panel/example_panel.hpp"
#include "panel_registry.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "widgets_showcase_panel/widgets_showcase_panel.hpp"

// Add one include + one Register(...) line per real panel as they get built:
// #include "ui/panels/properties_panel/properties_panel.hpp"

namespace ui::panels
{
    void RegisterAllPanels()
    {
        auto& registry = PanelRegistry::getInstance();

        registry.Register(ui::framework::wx::ids::panels::kExample,
                          [](wxWindow* parent) {
                              return new ExamplePanel(parent);
                          });

        registry.Register(ui::framework::wx::ids::panels::kWidgetsShowcase,
                          [](wxWindow* parent) {
                              return new WidgetsShowcasePanel(parent);
                          });
    }
} // namespace ui::panels
