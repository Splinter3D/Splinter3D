#include "panel_registration.hpp"

#include "panel_registry.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/ids/ids.hpp"

namespace ui::panels
{
    void RegisterAllPanels()
    {
        auto& registry = PanelRegistry::getInstance();

        registry.Register(ui::framework::wx::ids::panels::kModelViewer,
                          [](wxWindow* parent) -> wxPanel* {
                              return new ui::framework::occt::ModelViewerPanel(parent);
                          });
    }
} // namespace ui::panels
