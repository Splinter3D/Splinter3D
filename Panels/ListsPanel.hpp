#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>
#include <wx/statline.h>

class ListsPanel : public wxPanel
{
  public:
    explicit ListsPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        wxArrayString fruits;
        fruits.Add(_("Apple"));
        fruits.Add(_("Banana"));
        fruits.Add(_("Cherry"));
        fruits.Add(_("Durian"));
        fruits.Add(_("Elderberry"));

        auto* comboResult = new wxStaticText(this, wxID_ANY, _("Selected: (none)"));
        auto* combo       = wx::widgets::ComboBox(this)
                          .placeholder(_("Pick a fruit"))
                          .choices(fruits)
                          .onSelect([comboResult](const wxString& s) {
                              comboResult->SetLabel(_("Selected: ") + s);
                          })
                          .build();

        wxArrayString langs;
        langs.Add("C++");
        langs.Add("Python");
        langs.Add("Rust");
        langs.Add("Go");
        langs.Add("Zig");
        langs.Add("Java");

        auto* listBox  = wx::widgets::ListBox(this).choices(langs).multiSelect().height(120).build();
        auto* readList = wx::widgets::Button(this)
                             .label(_("Show selected languages"))
                             .onClick([listBox] {
                                 wxArrayInt sel;
                                 listBox->GetSelections(sel);
                                 if (sel.IsEmpty())
                                 {
                                     wx::widgets::info(_("Nothing selected"));
                                     return;
                                 }
                                 wxString out;
                                 for (int i : sel)
                                     out += listBox->GetString(i) + "\n";
                                 wx::widgets::info(out, _("Selected"));
                             })
                             .build();

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Lists & ComboBox showcase —")), 0, wxALL, 10);
        sizer->Add(new wxStaticText(this, wxID_ANY, _("ComboBox (dropdown):")), 0, wxLEFT, 8);
        sizer->Add(combo, 0, wxALL | wxEXPAND, 8);
        sizer->Add(comboResult, 0, wxALL, 8);
        sizer->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 8);
        sizer->Add(new wxStaticText(this, wxID_ANY, _("ListBox (multi-select, Ctrl+click):")), 0, wxLEFT, 8);
        sizer->Add(listBox, 0, wxALL | wxEXPAND, 8);
        sizer->Add(readList, 0, wxALL | wxEXPAND, 8);
        SetSizer(sizer);
    }
};
