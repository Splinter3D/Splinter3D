# windows/

Full top-level windows (MainWindow, SettingsWindow, AboutWindow...). One
subfolder per window, one clear self-contained UI flow per window.

## BaseWindow

Every window inherits from BaseWindow (ui/windows/base_window/), which
gives every window in the app the same predictable initialization sequence.
Concrete windows override only the hooks they need - the rest stay no-op.

Guaranteed order, always:

| # | Hook           | Typical use                                                      |
|---|----------------|------------------------------------------------------------------|
| 1 | InitMenuBar()  | Build and attach the menu bar                                    |
| 2 | InitStatusBar()| Create the status bar                                            |
| 3 | InitLayout()   | Create the layout skeleton (via LayoutRegistry)                  |
| 4 | InitToolBars() | Build toolbars and place them into the layout                    |
| 5 | InitPanels()   | Create panels (via PanelRegistry) and place them into the layout |
| 6 | BindEvents()   | Bind event handlers                                              |

InitLayout() intentionally runs before InitToolBars() and InitPanels():
the layout skeleton must already exist so toolbars and panels have
somewhere to be placed (layout_->AddTop(...), layout_->AddCenter(...), etc.).

## Creating a new window - step by step

1. Create a subfolder: ui/windows/my_window/my_window.hpp + .cpp.
2. Inherit from BaseWindow, override whichever hooks you need:

```cpp
// my_window.hpp
#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"
#include "ui/windows/base_window/base_window.hpp"

#include <memory>

namespace ui::windows
{
    class MyWindow : public BaseWindow
    {
    public:
        MyWindow();

    protected:
        void InitLayout() override;
        void InitPanels() override;

    private:
        std::unique_ptr<ui::layouts::BaseLayout> layout_;
    };
} // namespace ui::windows
```

```cpp
// my_window.cpp
#include "my_window.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registry.hpp"

namespace ui::windows
{
    MyWindow::MyWindow()
        : BaseWindow(nullptr, wxID_ANY, "My Window",
                     wxDefaultPosition, wxSize(800, 600))
    {
        InitializeAll(); // <-- see the critical rule below
    }

    void MyWindow::InitLayout()
    {
        layout_ = std::unique_ptr<ui::layouts::BaseLayout>(
            ui::layouts::LayoutRegistry::getInstance()
                .Create(ui::framework::wx::ids::layouts::kMain, this));
    }

    void MyWindow::InitPanels()
    {
        // e.g.
        // auto* panel = ui::panels::PanelRegistry::getInstance()
        //     .Create(ids::panels::kSomething, this);
        // layout_->AddCenter(panel);
    }
} // namespace ui::windows
```

3. That's it - no other file needs to change to introduce a new window.

## Critical rule: InitializeAll() must be called manually

BaseWindow's own constructor **cannot** call the hooks itself. Due to how
virtual dispatch works during construction, calling them from BaseWindow's
constructor would only ever reach the no-op versions defined there - never
your overrides in MyWindow.

So every concrete window **must** call InitializeAll() itself, as the very
last line of its own constructor (see MyWindow::MyWindow() above, and
MainWindow::MainWindow() for a real example). Forgetting this line means
none of your hooks ever run - the window will simply appear empty.

## Reference implementation

See ui/windows/main_window/ for a complete, working example: menu bar,
status bar, layout, toolbar, and panels all wired end-to-end through their
respective registries (see ui/layouts/README.md, ui/panels/README.md,
ui/toolbars/README.md, ui/menus/README.md).
