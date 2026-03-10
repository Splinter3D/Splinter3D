#pragma once

#include "../Wx/Occt/OcctCanvas.hpp"
#include "../Wx/Occt/OcctExporter.hpp"
#include "../Wx/Occt/OcctLoader.hpp"
#include "../Wx/Wx.hpp"

#include <BRepPrimAPI_MakeBox.hxx>
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

        addBtn(_("Open STL/STEP"), [this] { onOpen(); });
        addBtn(_("Cube test"), [this] { onCreateCube(); });
        toolbar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxLI_VERTICAL),
                     0, wxEXPAND | wxALL, 4);
        addBtn(_("Export STL bin"), [this] { onExport(false); });
        addBtn(_("Export STL ASCII"), [this] { onExport(true); });
        toolbar->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition,
                                      wxDefaultSize, wxLI_VERTICAL),
                     0, wxEXPAND | wxALL, 4);
        addBtn(_("Fit [F]"), [this] { m_canvas->fitAll(); });
        addBtn(_("Front [1]"), [this] { m_canvas->viewFront(); });
        addBtn(_("Right [3]"), [this] { m_canvas->viewRight(); });
        addBtn(_("Top   [7]"), [this] { m_canvas->viewTop(); });

        // ── Status bars ───────────────────────────────────────────────
        m_fileText = new wxStaticText(this, wxID_ANY, _("Aucun fichier."));
        m_selText  = new wxStaticText(this, wxID_ANY, _("Sélection : aucune  [clic gauche pour sélectionner]"));

        // ── Canvas 3D ─────────────────────────────────────────────────
        m_canvas = new wx::occt::OcctCanvas(this);
        m_canvas->onSelectionChanged([this](const std::string& name) {
            if (name.empty())
                m_selText->SetLabel(_("Sélection : aucune"));
            else
                m_selText->SetLabel(_("Sélection : ") + wxString::FromUTF8(name));
        });

        sizer->Add(toolbar, 0, wxALL | wxEXPAND, 4);
        sizer->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, 4);
        sizer->Add(m_fileText, 0, wxALL, 4);
        sizer->Add(m_selText, 0, wxALL, 4);
        sizer->Add(m_canvas, 1, wxEXPAND | wxALL, 0);
        SetSizer(sizer);
    }

  private:
    wx::occt::OcctCanvas* m_canvas    = nullptr;
    wxStaticText*         m_fileText  = nullptr;
    wxStaticText*         m_selText   = nullptr;
    int                   m_cubeCount = 0;

    // ── Ouvrir un fichier ─────────────────────────────────────────────
    void onOpen()
    {
        const wxString filter =
            _("STL et STEP|*.stl;*.stp;*.step"
              "|STL (*.stl)|*.stl"
              "|STEP (*.stp;*.step)|*.stp;*.step"
              "|Tous (*.*)|*.*");

        if (auto path = wx::widgets::openFile(this, _("Ouvrir fichier"), filter))
        {
            const std::string p = path->ToStdString();
            m_fileText->SetLabel(_("Chargement…"));
            Update();
            try
            {
                wx::occt::OcctLoader loader;
                TopoDS_Shape         shape = loader.load(p);

                // Nom = nom de fichier sans extension
                std::string name =
                    std::filesystem::path(p).stem().string();
                // Si le nom existe déjà, on numérote
                std::string base = name;
                int         n    = 1;
                while (!m_canvas->addShape(name, shape))
                    name = base + "_" + std::to_string(n++);

                m_fileText->SetLabel(_("Chargé : ") + wxString::FromUTF8(name));
            }
            catch (const std::exception& ex)
            {
                m_fileText->SetLabel(_("Erreur : ") + wxString::FromUTF8(ex.what()));
            }
        }
    }

    // ── Créer un cube de test ─────────────────────────────────────────
    void onCreateCube()
    {
        try
        {
            // BRepPrimAPI_MakeBox(dx, dy, dz) — cube 10x10x10
            TopoDS_Shape cube = BRepPrimAPI_MakeBox(10.0, 10.0, 10.0).Shape();

            std::string name = "Cube";
            std::string base = name;
            int         n    = 1;
            while (!m_canvas->addShape(name, cube))
                name = base + "_" + std::to_string(n++);

            m_fileText->SetLabel(_("Créé : ") + wxString::FromUTF8(name));
            m_canvas->fitAll();
        }
        catch (const std::exception& ex)
        {
            wxMessageBox(ex.what(), _("Erreur"), wxOK | wxICON_ERROR);
        }
    }

    // ── Exporter la sélection en STL ──────────────────────────────────
    void onExport(bool ascii)
    {
        const std::string& sel = m_canvas->selectedName();
        if (sel.empty())
        {
            wxMessageBox(_("Sélectionnez d'abord un objet (clic gauche)."),
                         _("Export STL"), wxOK | wxICON_INFORMATION);
            return;
        }

        const wxString filter = ascii
                                    ? _("STL ASCII (*.stl)|*.stl")
                                    : _("STL Binaire (*.stl)|*.stl");

        wxFileDialog dlg(this, _("Enregistrer STL"), "", wxString::FromUTF8(sel) + ".stl",
                         filter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
        if (dlg.ShowModal() != wxID_OK)
            return;

        try
        {
            wx::occt::OcctExporter::exportStl(
                m_canvas->selectedShape(),
                dlg.GetPath().ToStdString(),
                ascii);

            wxMessageBox(
                _("Exporté avec succès :\n") + dlg.GetPath(),
                _("Export STL"), wxOK | wxICON_INFORMATION);
        }
        catch (const std::exception& ex)
        {
            wxMessageBox(wxString::FromUTF8(ex.what()),
                         _("Erreur export"), wxOK | wxICON_ERROR);
        }
    }
};