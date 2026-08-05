#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>
#include <wx/statline.h>

class SlidersPanel : public wxPanel
{
  public:
    explicit SlidersPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* label = new wxStaticText(this, wxID_ANY, _("Slider value: 50"));
        auto* gauge = wx::widgets::Gauge(this).range(100).value(50).build();

        auto* slider = wx::widgets::Slider(this)
                           .range(0, 100)
                           .value(50)
                           .withLabels()
                           .onChange([label, gauge](int v) {
                               label->SetLabel(wxString::Format(_("Slider value: %d"), v));
                               gauge->SetValue(v);
                           })
                           .build();

        auto* vlabel  = new wxStaticText(this, wxID_ANY, _("Vertical: 30"));
        auto* vslider = wx::widgets::Slider(this)
                            .range(0, 100)
                            .value(30)
                            .vertical()
                            .withLabels()
                            .size(-1, 150)
                            .onChange([vlabel](int v) {
                                vlabel->SetLabel(wxString::Format(_("Vertical: %d"), v));
                            })
                            .build();

        auto* hSizer = new wxBoxSizer(wxHORIZONTAL);
        hSizer->Add(vslider, 0, wxALL, 8);
        hSizer->Add(vlabel, 0, wxALL | wxALIGN_CENTER_VERTICAL, 8);

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Sliders & Gauge showcase —")), 0, wxALL, 10);
        sizer->Add(label, 0, wxALL, 8);
        sizer->Add(slider, 0, wxALL | wxEXPAND, 8);
        sizer->Add(new wxStaticText(this, wxID_ANY, _("Gauge:")), 0, wxLEFT, 8);
        sizer->Add(gauge, 0, wxALL | wxEXPAND, 8);
        sizer->Add(new wxStaticLine(this), 0, wxALL | wxEXPAND, 8);
        sizer->Add(hSizer, 0, wxALL, 0);
        SetSizer(sizer);
    }
};
