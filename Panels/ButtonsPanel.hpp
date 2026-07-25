#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>
#include <wx/statline.h>

class ButtonsPanel : public wxPanel
{
  public:
    explicit ButtonsPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        auto* btn = wx::widgets::Button(this)
                        .label(_("Normal Button"))
                        .onClick([] { wx::widgets::info(_("You clicked a normal button!"), _("Button")); })
                        .build();

        // Self-referential label swap: bind after construction
        auto* toggle = wx::widgets::ToggleButton(this).label(_("Toggle me OFF")).build();
        toggle->Bind(wxEVT_TOGGLEBUTTON, [toggle](wxCommandEvent& e) {
            toggle->SetLabel(e.IsChecked() ? _("Toggle me ON") : _("Toggle me OFF"));
        });

        auto* disabled = wx::widgets::Button(this).label(_("Disabled Button")).disabled().build();

        auto* colorBtn = wx::widgets::Button(this)
                             .label(_("Pick a Color (dialog)"))
                             .onClick([this] {
                                 if (auto c = wx::widgets::pickColor(this))
                                     wx::widgets::info(
                                         wxString::Format(_("R=%d G=%d B=%d"),
                                                          c->Red(), c->Green(), c->Blue()),
                                         _("Color picked"));
                             })
                             .build();

        auto* fileBtn = wx::widgets::Button(this)
                            .label(_("Open File (dialog)"))
                            .onClick([this] {
                                if (auto path = wx::widgets::openFile(this))
                                    wx::widgets::info(_("Selected: ") + *path, _("File"));
                            })
                            .build();

        auto* progressBtn = wx::widgets::Button(this)
                                .label(_("Show Progress Dialog"))
                                .onClick([this] {
                                    wx::widgets::progress(this, _("Loading..."), 100,
                                                          [](wxProgressDialog& dlg, int i) {
                                                              dlg.Update(i, wxString::Format(_("Step %d/100"), i));
                                                              wxMilliSleep(50);
                                                          });
                                })
                                .build();

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Buttons showcase —")), 0, wxALL, 10);
        sizer->Add(btn, 0, wxALL | wxEXPAND, 8);
        sizer->Add(toggle, 0, wxALL | wxEXPAND, 8);
        sizer->Add(disabled, 0, wxALL | wxEXPAND, 8);
        sizer->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 8);
        sizer->Add(colorBtn, 0, wxALL | wxEXPAND, 8);
        sizer->Add(fileBtn, 0, wxALL | wxEXPAND, 8);
        sizer->Add(progressBtn, 0, wxALL | wxEXPAND, 8);
        SetSizer(sizer);
    }
};
