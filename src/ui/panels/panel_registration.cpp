#include "panel_registration.hpp"

#include "panel_registry.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/panels/side_dock/side_dock.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"

namespace ui::panels
{
    void RegisterAllPanels()
    {
        auto& registry = PanelRegistry::getInstance();

        registry.Register(ui::framework::wx::ids::panels::kModelViewer,
                          [](wxWindow* parent) -> wxPanel* {
                              return new ui::framework::occt::ModelViewerPanel(parent);
                          });

        registry.Register(ui::framework::wx::ids::panels::kTasks,
                          [](wxWindow* parent) -> wxPanel* { return new TasksPanel(parent); });

        registry.Register(ui::framework::wx::ids::panels::kSideDock,
                          [](wxWindow* parent) -> wxPanel* { return new SideDock(parent); });
    }
} // namespace ui::panels
