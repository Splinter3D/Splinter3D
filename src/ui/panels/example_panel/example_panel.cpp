#include "example_panel.hpp"

namespace ui::panels
{
    ExamplePanel::ExamplePanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* label = new wxStaticText(this, wxID_ANY,
                                       "ExamplePanel - registry pipeline OK");
        sizer->Add(label, 0, wxALL | wxALIGN_CENTER, 20);
        SetSizer(sizer);
    }
} // namespace ui::panels
