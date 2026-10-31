# layouts/

Spatial organization of widgets (MainLayout, ViewerLayout, SettingsLayout...).
One subfolder per layout.

## Role of a Layout

A Layout answers exactly one question: **where does each widget go?**

For example, MainLayout organizes the window like this:

+---------------------------------------------------+
| MenuBar                                           |
+---------------------------------------------------+
| Toolbar                                           |
+---------------------------------------------------+
| Left     | Center               | Right           |
|          |                      |                 |
+---------------------------------------------------+
| Bottom                                            |
+---------------------------------------------------+

A layout:

- Only knows about wxSizer and wxWindow.
- Does not create panels, toolbars, or menus.
- Does not know their concrete type (ExplorerPanel, ViewerPanel...).
- Only places widgets it is handed.

It never knows about a command, a Mesh, a Renderer, or anything from
application/, geometry/, or rendering/.

## BaseLayout

Every layout inherits from BaseLayout (ui/layouts/base_layout/), which
declares a small, generic, position-based API rather than named slots:

```cpp
class BaseLayout
{
public:
    explicit BaseLayout(wxWindow* parent);

    virtual void AddTop(wxWindow* widget);
    virtual void AddLeft(wxWindow* widget);
    virtual void AddCenter(wxWindow* widget);
    virtual void AddRight(wxWindow* widget);
    virtual void AddBottom(wxWindow* widget);
};
```

All hooks default to no-op, so a concrete layout only overrides the slots
it actually uses (a SettingsLayout might only need AddCenter(), for
instance).

Using AddLeft()/AddCenter()/AddRight() instead of named methods like
SetExplorer()/SetViewer()/SetProperties() means the layout stays agnostic
of what occupies each slot. Tomorrow, "Viewer" can become "OCCT Viewer" or
"OpenGL Viewer" without touching the layout at all.

## LayoutRegistry

Layouts are never instantiated directly by a window. Instead, every layout
registers a factory into LayoutRegistry (a domain alias of the generic
WidgetRegistry<T> from ui/framework/wx/registry/ - that generic piece is
shared plumbing, not something you need to touch when just adding a layout).

Layouts take a single wxWindow* parent at construction time, exactly like
panels and toolbars, so they reuse WidgetRegistry<T> rather than
ObjectRegistry<T>.

Windows then ask the registry for the layout they need in their own
InitLayout(). This means:

- Adding a new layout never requires modifying existing windows.
- Different windows can use entirely different spatial organizations.
- Layout creation stays separated from window construction.


```
RegisterAllLayouts() (bootstrap.cpp, once, at startup)
│
└── registry.Register(id, factory) for every layout

MainWindow::InitLayout() / OtherWindow::InitLayout()
│
└── LayoutRegistry::getInstance().Create(id, parent) only when needed
```

Register() only stores a construction recipe (std::function<BaseLayout*(wxWindow*)>).
Nothing is created until a window calls Create().

## Why InitLayout() runs before InitToolBars() and InitPanels()

See ui/windows/README.md for the full guaranteed hook order. In short:
InitLayout() must run first so that layout_ already exists by the time
InitToolBars() and InitPanels() want to place their widgets into it
(layout_->AddTop(toolbar), layout_->AddCenter(panel), etc.).

## Creating a new layout - step by step

1. Reserve an id in ui/framework/wx/ids/ids.hpp, under the layouts
   namespace:

```cpp
namespace layouts
{
    constexpr int kMain = app::kBase + 900;
    constexpr int kViewer = app::kBase + 901; // <-- new
} // namespace layouts
```

2. Create a subfolder:

```
ui/layouts/viewer_layout/
    viewer_layout.hpp
    viewer_layout.cpp
```

Inherit from BaseLayout and override whichever slots you need:

```cpp
// viewer_layout.hpp
#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"
#include <wx/sizer.h>

namespace ui::layouts
{
    class ViewerLayout : public BaseLayout
    {
    public:
        explicit ViewerLayout(wxWindow* parent);

        void AddCenter(wxWindow* widget) override;
        void AddBottom(wxWindow* widget) override;

    private:
        wxBoxSizer* rootSizer_ = nullptr;
    };
} // namespace ui::layouts
```

```cpp
// viewer_layout.cpp
#include "viewer_layout.hpp"

namespace ui::layouts
{
    ViewerLayout::ViewerLayout(wxWindow* parent)
        : BaseLayout(parent)
    {
        rootSizer_ = new wxBoxSizer(wxVERTICAL);
        parent_->SetSizer(rootSizer_);
    }

    void ViewerLayout::AddCenter(wxWindow* widget)
    {
        if (widget == nullptr) return;
        rootSizer_->Add(widget, 1, wxEXPAND);
    }

    void ViewerLayout::AddBottom(wxWindow* widget)
    {
        if (widget == nullptr) return;
        rootSizer_->Add(widget, 0, wxEXPAND);
    }
} // namespace ui::layouts
```

3. Register its factory in layout_registration.cpp:

```cpp
#include "ui/layouts/viewer_layout/viewer_layout.hpp"

// inside RegisterAllLayouts():

registry.Register(ui::framework::wx::ids::layouts::kViewer,
                   [](wxWindow* parent)
                   {
                       return new ViewerLayout(parent);
                   });
```

4. Consume it from any window's InitLayout():

```cpp
layout_ = std::unique_ptr<ui::layouts::BaseLayout>(
    ui::layouts::LayoutRegistry::getInstance()
        .Create(ui::framework::wx::ids::layouts::kViewer, this));
```

That's the whole loop - no other file needs to change.

## Reference implementation

ui/layouts/main_layout/ (MainLayout) is the reference implementation: a
vertical root sizer holding an optional top slot, a horizontal middle band
(left | center | right), and an optional bottom slot. See
ui/windows/main_window/ for how MainWindow wires it up.
