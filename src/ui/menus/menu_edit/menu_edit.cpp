#include "menu_edit.hpp"

#include "ui/framework/wx/ids/ids.hpp"
#include "ui/framework/wx/locale/locale.hpp"

using ui::framework::wx::locale::tr;

namespace ui::menus
{
    MenuEdit::MenuEdit()
    {
        using ui::framework::wx::ids::history::kRedo;
        using ui::framework::wx::ids::history::kUndo;

        Append(kUndo, tr("menu.edit.undo"));
        Append(kRedo, tr("menu.edit.redo"));
    }

} // namespace ui::menus
