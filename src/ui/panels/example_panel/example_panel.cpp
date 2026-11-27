#include "example_panel.hpp"

#include "ui/framework/wx/locale/locale.hpp"

using ui::framework::wx::locale::tr;

namespace ui::panels
{
    ExamplePanel::ExamplePanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* label = new wxStaticText(this, wxID_ANY,
                                       tr("showcase.example"));
        sizer->Add(label, 0, wxALL | wxALIGN_CENTER, 20);
        SetSizer(sizer);
    }
} // namespace ui::panels
