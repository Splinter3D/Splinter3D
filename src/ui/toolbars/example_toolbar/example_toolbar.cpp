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
                    wxTB_HORIZONTAL | wxTB_FLAT)
    {
        using ui::framework::wx::ids::file::kNew;
        using ui::framework::wx::ids::file::kOpen;
        using ui::framework::wx::ids::file::kSave;

        // Temporary tools used to validate toolbar creation and event binding.
        //
        // wxArtProvider supplies standard platform-native icons, avoiding the
        // need for image assets while the UI infrastructure is being built.
        AddTool(
            kNew,
            "New",
            wxArtProvider::GetBitmap(wxART_NEW, wxART_TOOLBAR));

        AddTool(
            kOpen,
            "Open",
            wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_TOOLBAR));

        AddTool(
            kSave,
            "Save",
            wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR));

        // Finalize the native toolbar after all tools have been added.
        Realize();
    }

} // namespace ui::toolbars
