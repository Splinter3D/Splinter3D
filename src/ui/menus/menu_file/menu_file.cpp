#include "menu_file.hpp"

#include "ui/framework/wx/ids/ids.hpp"

namespace ui::menus
{
    MenuFile::MenuFile()
    {
        using ui::framework::wx::ids::file::kExit;
        using ui::framework::wx::ids::file::kNew;
        using ui::framework::wx::ids::file::kOpen;
        using ui::framework::wx::ids::file::kSave;

        Append(kNew, "&New\tCtrl+N");
        Append(kOpen, "&Open\tCtrl+O");
        Append(kSave, "&Save\tCtrl+S");

        AppendSeparator();

        Append(kOpen, "&Import...");
        Append(kSave, "&Export...");

        AppendSeparator();

        Append(kExit, "E&xit\tAlt+F4");
    }

} // namespace ui::menus
