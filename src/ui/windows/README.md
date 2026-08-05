# windows/

Full top-level windows (`MainWindow`, `SettingsWindow`, `AboutWindow`...). One
subfolder per window, one clear self-contained UI flow per window.

## BaseWindow

Every window inherits from `BaseWindow` (`ui/windows/base_window/`), which
gives every window in the app the same predictable initialization sequence.
Concrete windows override only the hooks they need - the rest stay no-op.

Guaranteed order, always:

| # | Hook             | Typical use                                   |
|---|------------------|------------------------------------------------|
| 1 | `InitMenuBar()`  | Build and attach the menu bar                   |
| 2 | `InitToolBars()` | Build and attach toolbars                       |
| 3 | `InitStatusBar()`| Create the status bar                           |
| 4 | `InitLayout()`   | Create sizers / layout skeleton                 |
| 5 | `InitPanels()`   | Create panels (via `PanelRegistry`) into layout |
| 6 | `BindEvents()`   | Bind event handlers                             |

## Creating a new window - step by step

1. Create a subfolder: `ui/windows/my_window/my_window.hpp` + `.cpp`.
2. Inherit from `BaseWindow`, override whichever hooks you need:

```cpp
// my_window.hpp
#pragma once

#include "ui/windows/base_window/base_window.hpp"

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
        wxBoxSizer* rootSizer_ = nullptr;
    };
} // namespace ui::windows
```

```cpp
// my_window.cpp
#include "my_window.hpp"

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
        rootSizer_ = new wxBoxSizer(wxVERTICAL);
        SetSizer(rootSizer_);
    }

    void MyWindow::InitPanels()
    {
        // e.g. rootSizer_->Add(
        //     ui::panels::PanelRegistry::getInstance()
        //         .Create(ids::panels::kSomething, this),
        //     1, wxEXPAND);
    }
} // namespace ui::windows
```

3. That's it - no other file needs to change to introduce a new window.

## Critical rule: `InitializeAll()` must be called manually

`BaseWindow`'s own constructor **cannot** call the hooks itself. Due to how
virtual dispatch works during construction, calling them from `BaseWindow`'s
constructor would only ever reach the no-op versions defined there - never
your overrides in `MyWindow`.

So every concrete window **must** call `InitializeAll()` itself, as the very
last line of its own constructor (see `MyWindow::MyWindow()` above, and
`MainWindow::MainWindow()` for a real example). Forgetting this line means
none of your hooks ever run - the window will simply appear empty.

## Reference implementation

See `ui/windows/main_window/` for a complete, working example: menu bar not
yet built, but `InitStatusBar()`, `InitLayout()`, and `InitPanels()` are all
implemented and wired to `PanelRegistry` (see `ui/panels/README.md`).
