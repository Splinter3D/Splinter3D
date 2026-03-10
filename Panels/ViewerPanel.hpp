#pragma once

#include "../Wx/Occt/OcctCanvas.hpp"
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>
#include <wx/statline.h>

class ViewerPanel : public wxPanel
{
  public:
    explicit ViewerPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);

        // ── Toolbar ──────────────────────────────────────────────────
        auto* toolbar = new wxBoxSizer(wxHORIZONTAL);

        auto addBtn = [&](const wxString& lbl, std::function<void()> fn) {
            toolbar->Add(
                wx::widgets::Button(this).label(lbl).onClick(std::move(fn)).build(),
                0, wxALL, 3);
        };

        addBtn(_("Open  [O]"), [this] { onOpenFile(); });
        toolbar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxLI_VERTICAL),
                     0, wxEXPAND | wxALL, 4);
        addBtn(_("Fit All  [F]"), [this] { if (m_canvas) m_canvas->fitAll(); });
        toolbar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxLI_VERTICAL),
                     0, wxEXPAND | wxALL, 4);
        addBtn(_("Front [Num1]"), [this] { if (m_canvas) m_canvas->viewFront(); });
        addBtn(_("Right [Num3]"), [this] { if (m_canvas) m_canvas->viewRight(); });
        addBtn(_("Top   [Num7]"), [this] { if (m_canvas) m_canvas->viewTop(); });

        m_statusText = new wxStaticText(this, wxID_ANY, _("Aucun fichier chargé."));

        m_canvas = new wx::occt::OcctCanvas(this);

        sizer->Add(toolbar, 0, wxALL | wxEXPAND, 4);
        sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        sizer->Add(m_statusText, 0, wxALL, 4);
        sizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);
        SetSizer(sizer);
    }

  private:
    wx::occt::OcctCanvas* m_canvas     = nullptr;
    wxStaticText*         m_statusText = nullptr;

    void onOpenFile()
    {
        const wxString filter =
            _("Fichiers CAD (*.stp;*.step;*.stl;*.igs;*.iges)"
              "|*.stp;*.step;*.stl;*.igs;*.iges"
              "|STEP (*.stp;*.step)|*.stp;*.step"
              "|STL  (*.stl)|*.stl"
              "|IGES (*.igs;*.iges)|*.igs;*.iges"
              "|Tous (*.*)|*.*");

        if (auto path = wx::widgets::openFile(this, _("Ouvrir fichier CAD"), filter))
        {
            m_statusText->SetLabel(_("Chargement: ") + *path + _(" …"));
            Update();

            if (m_canvas->loadFile(path->ToStdString()))
                m_statusText->SetLabel(_("Chargé: ") + *path);
            else
                m_statusText->SetLabel(_("Échec: ") + *path);
        }
    }
};