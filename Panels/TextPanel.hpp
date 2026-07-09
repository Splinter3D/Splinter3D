#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>

class TextPanel : public wxPanel
{
  public:
    explicit TextPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer      = new wxBoxSizer(wxVERTICAL);
        auto* singleLine = wx::widgets::TextInput(this).hint(_("Single-line input...")).build();
        auto* password   = wx::widgets::TextInput(this).hint(_("Password input...")).password().build();
        auto* multiLine  = wx::widgets::TextInput(this)
                              .value(_("Multi-line text area.\nYou can type here freely."))
                              .multiline()
                              .height(100)
                              .build();
        auto* readOnly = wx::widgets::TextInput(this).value(_("This is read-only text.")).readOnly().build();
        auto* spin     = wx::widgets::SpinInput(this).range(0, 100).value(0).build();

        auto* submit = wx::widgets::Button(this)
                           .label(_("Show entered values"))
                           .onClick([singleLine, password, spin] {
                               wx::widgets::info(
                                   wxString::Format(_("Text: %s\nPassword: %s\nNumber: %d"),
                                                    singleLine->GetValue(),
                                                    password->GetValue(),
                                                    spin->GetValue()),
                                   _("Values"));
                           })
                           .build();

        // Helper: label + control row
        auto add = [&](const wxString& lbl, wxWindow* ctrl) {
            sizer->Add(new wxStaticText(this, wxID_ANY, lbl), 0, wxLEFT | wxTOP, 8);
            sizer->Add(ctrl, 0, wxALL | wxEXPAND, 8);
        };

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Text inputs showcase —")), 0, wxALL, 10);
        add(_("Single line:"), singleLine);
        add(_("Password:"), password);
        add(_("Multi-line:"), multiLine);
        add(_("Read-only:"), readOnly);
        add(_("Numeric (SpinCtrl):"), spin);
        sizer->Add(submit, 0, wxALL | wxEXPAND, 8);
        SetSizer(sizer);
    }
};
