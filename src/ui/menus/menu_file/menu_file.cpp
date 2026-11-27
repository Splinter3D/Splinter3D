#include "menu_file.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/locale/locale.hpp"

using ui::framework::wx::locale::tr;

namespace ui::menus
{
    MenuFile::MenuFile()
    {
        using ui::framework::wx::ids::file::kExit;
        using ui::framework::wx::ids::file::kNew;
        using ui::framework::wx::ids::file::kOpen;
        using ui::framework::wx::ids::file::kSave;

        Append(kNew, tr("menu.file.new"));
        Append(kOpen, tr("menu.file.open"));
        Append(kSave, tr("menu.file.save"));

        AppendSeparator();

        Append(kOpen, tr("menu.file.import"));
        Append(kSave, tr("menu.file.export"));

        AppendSeparator();

        Append(kExit, tr("menu.file.exit"));
    }

} // namespace ui::menus
