# toolbars/

Application toolbars (`MainToolbar`, `SelectionToolbar`,
`ViewToolbar`...).

Each toolbar has its own subfolder and contains only the visual definition
of the toolbar and its tools.

## ToolBarRegistry

Toolbars are never instantiated directly by windows. Instead, every toolbar
registers a factory into `ToolBarRegistry` (a domain alias of the generic
`WidgetRegistry<T>` from `ui/framework/wx/registry/` - that generic piece is
shared plumbing, not something you need to touch when just adding a toolbar).

Toolbars are `wxWindow`-derived objects, therefore they require a parent
window and use `WidgetRegistry<T>`.

Windows then ask the registry for the toolbar(s) they need in their own
`InitToolBars()`.

This means:

- Adding a new toolbar never requires modifying existing windows.
- Different windows can expose different toolbars.
- Toolbars can later be managed by wxAUI docking because they remain regular
  wxWindow instances.


```
RegisterAllToolBars() (bootstrap.cpp, once, at startup)
│
└── registry.Register(id, factory) for every toolbar

MainWindow::InitToolBars() / OtherWindow::InitToolBars()
│
└── ToolBarRegistry::getInstance().Create(id, parent)
only when needed
```


`Register()` only stores a construction recipe (`std::function<ObjectT*()>`).
Nothing is created or displayed until a window calls `Create()`.

## Creating a new toolbar - step by step

1. Reserve an id in `ui/framework/wx/ids/ids.hpp`, under the `toolbars`
   namespace.

Example:

```cpp
namespace toolbars
{
    constexpr int kExample = app::kBase + 800;
    constexpr int kView    = app::kBase + 801; // <-- new
} // namespace toolbars
```

2. Create a subfolder:

```
ui/toolbars/view_toolbar/
    view_toolbar.hpp
    view_toolbar.cpp
```

Create a normal wxToolBar subclass:

```cpp
// view_toolbar.hpp
#pragma once

#include <wx/toolbar.h>

namespace ui::toolbars
{
    class ViewToolbar : public wxToolBar
    {
    public:
        explicit ViewToolbar(wxWindow* parent);
    };
} // namespace ui::toolbars
```

```cpp
// view_toolbar.cpp
#include "view_toolbar.hpp"

namespace ui::toolbars
{
    ViewToolbar::ViewToolbar(wxWindow* parent)
        : wxToolBar(parent, wxID_ANY)
    {
        AddTool(wxID_REFRESH, "Refresh");

        Realize();
    }
} // namespace ui::toolbars
```

3. Register its factory in toolbar_registration.cpp:

```cpp
#include "ui/toolbars/view_toolbar/view_toolbar.hpp"

// inside RegisterAllToolBars():

registry.Register(ui::framework::wx::ids::toolbars::kView,
                   [](wxWindow* parent)
                   {
                       return new ViewToolbar(parent);
                   });
```

4. Consume it from any window's InitToolBars():

```cpp
auto* toolbar =
    ToolBarRegistry::getInstance()
        .Create(ui::framework::wx::ids::toolbars::kView, this);

if (toolbar != nullptr)
{
    rootSizer_->Add(toolbar, 0, wxEXPAND);
}
```

That's the whole loop - no other file needs to change.

Reference implementation

`ui/toolbars/example_toolbar/` (`ExampleToolBar`) is a minimal, working
toolbar kept around purely to show the pattern end-to-end.

It can safely be deleted - along with its `Register(...)` call in
`toolbar_registration.cpp` and its id in `ids.hpp` - once the first real
toolbar exists.
