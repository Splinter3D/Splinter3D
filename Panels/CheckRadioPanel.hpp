#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>
#include <wx/statline.h>

class CheckRadioPanel : public wxPanel
{
  public:
    explicit CheckRadioPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* cb1   = wx::widgets::CheckBox(this).label(_("Option Alpha")).build();
        auto* cb2   = wx::widgets::CheckBox(this).label(_("Option Beta")).build();
        auto* cb3   = wx::widgets::CheckBox(this).label(_("Option Gamma (disabled)")).disabled().build();

        wxArrayString modes;
        modes.Add(_("Mode A"));
        modes.Add(_("Mode B"));
        modes.Add(_("Mode C"));

        auto* result = new wxStaticText(this, wxID_ANY, _("Current mode: Mode A"));
        auto* radio  = wx::widgets::RadioBox(this)
                          .title(_("Select a mode"))
                          .choices(modes)
                          .columns(1)
                          .onChange([result](const wxString& sel) {
                              result->SetLabel(_("Current mode: ") + sel);
                          })
                          .build();

        auto* readBtn = wx::widgets::Button(this)
                            .label(_("Read selections"))
                            .onClick([cb1, cb2, radio] {
                                wx::widgets::info(
                                    wxString::Format(_("Alpha: %s\nBeta: %s\nMode: %s"),
                                                     cb1->IsChecked() ? _("ON") : _("OFF"),
                                                     cb2->IsChecked() ? _("ON") : _("OFF"),
                                                     radio->GetStringSelection()),
                                    _("Selections"));
                            })
                            .build();

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Checkboxes & Radio showcase —")), 0, wxALL, 10);
        sizer->Add(cb1, 0, wxALL, 8);
        sizer->Add(cb2, 0, wxALL, 8);
        sizer->Add(cb3, 0, wxALL, 8);
        sizer->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 8);
        sizer->Add(radio, 0, wxALL, 8);
        sizer->Add(result, 0, wxALL, 8);
        sizer->Add(readBtn, 0, wxALL | wxEXPAND, 8);
        SetSizer(sizer);
    }
};
