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

## Side dock

`ui/panels/side_dock/` (`SideDock`, id `panels::kSideDock`) sits on the right
of the viewer: a strip of toggle buttons, one per page, and the open page (at
most one) beside it. `AddPage(page, icon, label)` adds a page (created as a
child of the dock) and its icon button, the label as tooltip (or as text when
the icon is null). A button opens its page or closes it.

Pages can also open themselves: `RequestOpen(page)` opens it if nothing is
open, and `ReleaseOpen(page)` closes it again - unless the user opened it by
hand (the button or `Open()`), in which case it stays until closed by hand.
Once the user has closed the dock by hand (the button or `Close()`), requests
no longer open it; the user can still open it.

## Tasks panel

`ui/panels/tasks_panel/` (`TasksPanel`, id `panels::kTasks`) lists the
background tasks of an `application::tasks::TaskManager` as cards (name,
progress bar, status, Cancel or Dismiss), or a placeholder when there is none.
`attach(manager)` makes it follow that manager; `SetOnBusyChanged` reports its
first card appearing and its last one going. `MainWindow` adds it as a
`SideDock` page and turns those reports into `RequestOpen` / `ReleaseOpen`, so
it pops up while tasks run and closes after, unless opened by hand - and no
longer pops up once the user has closed it.
Auto-opening can be turned off with the `tasks.auto_open` setting.

## Settings panel

`ui/panels/settings_panel/` (`SettingsPanel`, id `panels::kSettings`) is the
dock's second page. It lists every setting declared in
`platform::settings::SettingsRegistry`, grouped by section in declaration
order: a checkbox per Toggle, a drop-down per Choice, and per KeyBind a
`ui::widgets::KeyBindButton` (click, then press the combination; Escape
cancels) with a reset button. Combinations used twice are flagged. They are
stored as `wxAcceleratorEntry` raw strings where `Ctrl` is the primary
modifier (Command on macOS) and `RawCtrl` the macOS Control key; on macOS they
show as symbols (`⌃⌥⇧⌘`). Settings
declared after it was built show the next time it is shown.

To add a setting, declare it where it is used and read it from there:

```cpp
auto& settings = platform::settings::SettingsRegistry::getInstance();
subscription_  = settings.add({"my.key", "settings.section.general", "settings.my.label", "",
                               platform::settings::Kind::Toggle, "1", {}},
                              [this](const std::string& value) { /* react */ });
if (settings.isOn("my.key")) { ... }
```

Current settings: language (`LocaleManager`, applies after a restart), tasks
auto-open (`MainWindow`), mesh precision and B-Rep on import
(`ModelViewerPanel`, which offers to convert the loaded models when the
precision changes or B-Reps on import are turned on), and the keyboard shortcuts (`ShortcutEvents`, also shown on the File
menu items).
