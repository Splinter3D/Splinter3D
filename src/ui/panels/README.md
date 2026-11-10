# panels/

Workspaces within a window (`ExplorerPanel`, `ViewerPanel`, `PropertiesPanel`,
`ConsolePanel`...). One subfolder per panel.

## PanelRegistry

Panels are never instantiated directly by a window. Instead, every panel
registers a factory into `PanelRegistry` (a domain alias of the generic
`WidgetRegistry<T>` from `ui/framework/wx/registry/` - that generic piece is
shared plumbing, not something you need to touch when just adding a panel).

Windows then ask the registry for the panel(s) they need, by id, in their own
`InitPanels()`. This means:

- Adding a new panel never requires touching any existing window.
- A given window only pulls in the panel ids it actually cares about - two
  different windows can show entirely different sets of panels without
  knowing about each other.

```
RegisterAllPanels()  (bootstrap.cpp, once, at startup)
   │
   └── registry.Register(id, factory)   for every panel

MainWindow::InitPanels() / SettingsWindow::InitPanels() / ...
   │
   └── PanelRegistry::getInstance().Create(id, parent)   only when needed
```

`Register()` just stores a recipe (a `std::function`). Nothing is built or
shown until some window actually calls `Create()` for that id.

## Creating a new panel - step by step

1. Reserve an id in `ui/framework/wx/ids/ids.hpp`, under the `panels`
   namespace (range `app::kBase + 600` and up):

```cpp
namespace panels
{
    constexpr int kExample    = app::kBase + 600;
    constexpr int kProperties = app::kBase + 601; // <-- new
} // namespace panels
```

2. Create a subfolder: `ui/panels/properties_panel/properties_panel.hpp` + `.cpp`,
   a plain `wxPanel` subclass:

```cpp
// properties_panel.hpp
#pragma once

#include <wx/wx.h>

namespace ui::panels
{
    class PropertiesPanel : public wxPanel
    {
    public:
        explicit PropertiesPanel(wxWindow* parent);
    };
} // namespace ui::panels
```

```cpp
// properties_panel.cpp
#include "properties_panel.hpp"

namespace ui::panels
{
    PropertiesPanel::PropertiesPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        // build the panel's own sizer/content here
    }
} // namespace ui::panels
```

3. Register its factory in `panel_registration.cpp`:

```cpp
#include "ui/panels/properties_panel/properties_panel.hpp"

// inside RegisterAllPanels():
registry.Register(ui::framework::wx::ids::panels::kProperties,
                   [](wxWindow* parent)
                   {
                       return new PropertiesPanel(parent);
                   });
```

4. Consume it from any window's `InitPanels()`:

```cpp
auto* properties = PanelRegistry::getInstance()
    .Create(ui::framework::wx::ids::panels::kProperties, this);
if (properties != nullptr)
{
    rootSizer_->Add(properties, 1, wxEXPAND);
}
```

That's the whole loop - no other file needs to change.

## Reference implementation

`ui/panels/example_panel/` (`ExamplePanel`) is a minimal, working panel kept
around purely to show the pattern end-to-end. Safe to delete - along with
its `Register(...)` call in `panel_registration.cpp` and its id in
`ids.hpp` - once a first real panel exists.
