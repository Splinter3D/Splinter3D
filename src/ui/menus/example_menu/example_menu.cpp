#include "example_menu.hpp"

#include "ui/framework/wx/ids/ids.hpp"

namespace ui::menus
{
    ExampleMenu::ExampleMenu()
    {
        using ui::framework::wx::ids::file::kExit;
        using ui::framework::wx::ids::file::kNew;
        using ui::framework::wx::ids::file::kOpen;

        // Temporary entries used only to validate menu creation and
        // event binding during early development.
        Append(kNew, "&New\tCtrl+N");
        Append(kOpen, "&Open\tCtrl+O");

        AppendSeparator();

        Append(kExit, "E&xit\tAlt+F4");
    }

} // namespace ui::menus
