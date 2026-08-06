# menus/

Application menus (`File`, `Edit`, `View`, `Help`...).
Each menu has its own subfolder and is responsible only for defining
its entries.

## MenuRegistry

Menus are never instantiated directly by windows. Instead, every menu
registers a factory into `MenuRegistry` (a domain alias of the generic
`ObjectRegistry<T>` from `ui/framework/wx/registry/` - that generic piece is
shared plumbing, not something you need to touch when just adding a menu).

Unlike panels or toolbars, menus are not `wxWindow` objects and do not have
a parent at construction time. Therefore they use `ObjectRegistry<T>` instead
of `WidgetRegistry<T>`.

Windows then ask the registry for the menus they need when creating their
`wxMenuBar`.

This means:

- Adding a new menu never requires modifying the registry itself.
- Different windows can assemble different menu bars.
- Menu creation stays separated from window construction.

```
RegisterAllMenus() (bootstrap.cpp, once, at startup)
│
└── registry.Register(id, factory) for every menu

MainWindow::InitMenuBar() / SettingsWindow::InitMenuBar() / ...
│
└── MenuRegistry::getInstance().Create(id) only when needed
```

`Register()` only stores a recipe (`std::function`). Nothing is created until
a window explicitly calls `Create()` for that id.

## Creating a new menu - step by step

1. Reserve an id in `ui/framework/wx/ids/ids.hpp`, under the `menus`
   namespace.

Example:

```cpp
namespace menus
{
    constexpr int kExample = app::kBase + 700;
    constexpr int kHelp    = app::kBase + 701; // <-- new
} // namespace menus
```

2. Create a subfolder:

```
ui/menus/help_menu/
    help_menu.hpp
    help_menu.cpp
```

Create a normal wxMenu subclass:

```cpp
// help_menu.hpp
#pragma once

#include <wx/menu.h>

namespace ui::menus
{
    class HelpMenu : public wxMenu
    {
    public:
        HelpMenu();
    };
} // namespace ui::menus
```

```cpp
// help_menu.cpp
#include "help_menu.hpp"

namespace ui::menus
{
    HelpMenu::HelpMenu()
    {
        Append(wxID_ABOUT, "&About");
    }
} // namespace ui::menus
```

3. Register its factory in menu_registration.cpp:

```cpp
#include "ui/menus/help_menu/help_menu.hpp"

// inside RegisterAllMenus():

registry.Register(ui::framework::wx::ids::menus::kHelp,
                   []()
                   {
                       return new HelpMenu();
                   });
```

4. Consume it from any window's InitMenuBar():

```cpp
auto* helpMenu =
    MenuRegistry::getInstance()
        .Create(ui::framework::wx::ids::menus::kHelp);

if (helpMenu != nullptr)
{
    menuBar->Append(helpMenu, "&Help");
}
```

That's the whole loop - no other file needs to change.

## Reference implementation

`ui/menus/example_menu/` (`ExampleMenu`) is a minimal, working menu kept
around purely to show the pattern end-to-end.

It can safely be deleted - along with its `Register(...)` call in
`menu_registration.cpp` and its id in `ids.hpp` - once the first real menu
is implemented.
