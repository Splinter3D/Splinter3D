#include "example_toolbar.hpp"

#include "ui/framework/wx/ids/ids.hpp"

#include <wx/artprov.h>

namespace ui::toolbars
{
    ExampleToolBar::ExampleToolBar(wxWindow* parent)
        : wxToolBar(parent,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxTB_HORIZONTAL | wxTB_FLAT | wxTB_TEXT | wxTB_NODIVIDER)
    {
        SetToolBitmapSize(wxSize(26, 26));
        SetMargins(8, 4);
        SetToolPacking(8);
        SetToolSeparation(4);

        using ui::framework::wx::ids::file::kNew;
        using ui::framework::wx::ids::file::kOpen;
        using ui::framework::wx::ids::file::kSave;

        AddTool(kNew,
                "New",
                wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR),
                "New model (Ctrl+N)");

        AddTool(kOpen,
                "Import",
                wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR),
                "Import STL model (Ctrl+O)");

        AddTool(kSave,
                "Export",
                wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR),
                "Export STL model (Ctrl+S)");

        // Finalize the native toolbar after all tools have been added.
        Realize();
    }

} // namespace ui::toolbars
