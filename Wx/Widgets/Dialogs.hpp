#pragma once
#include <functional>
#include <optional>
#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/numdlg.h>
#include <wx/progdlg.h>
#include <wx/textdlg.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  Message boxes
    // ─────────────────────────────────────────────
    inline void info(const wxString& msg, const wxString& title = _("Info"))
    {
        wxMessageBox(msg, title, wxOK | wxICON_INFORMATION);
    }

    inline void warning(const wxString& msg, const wxString& title = _("Warning"))
    {
        wxMessageBox(msg, title, wxOK | wxICON_WARNING);
    }

    inline void error(const wxString& msg, const wxString& title = _("Error"))
    {
        wxMessageBox(msg, title, wxOK | wxICON_ERROR);
    }

    inline bool confirm(const wxString& msg, const wxString& title = _("Question"))
    {
        return wxMessageBox(msg, title, wxYES_NO | wxICON_QUESTION) == wxYES;
    }

    // ─────────────────────────────────────────────
    //  Input dialogs — return std::optional (empty = cancelled)
    // ─────────────────────────────────────────────
    inline std::optional<wxString> askText(wxWindow*       parent,
                                           const wxString& prompt,
                                           const wxString& title        = _("Input"),
                                           const wxString& defaultValue = "")
    {
        wxTextEntryDialog dlg(parent, prompt, title, defaultValue);
        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetValue();
        return std::nullopt;
    }

    inline std::optional<long> askNumber(wxWindow*       parent,
                                         const wxString& prompt,
                                         const wxString& title        = _("Input"),
                                         long            defaultValue = 0,
                                         long            min          = 0,
                                         long            max          = 1000)
    {
        wxNumberEntryDialog dlg(parent, prompt, _("Number:"), title, defaultValue, min, max);
        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetValue();
        return std::nullopt;
    }

    inline std::optional<wxColour> pickColor(wxWindow* parent)
    {
        wxColourDialog dlg(parent);
        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetColourData().GetColour();
        return std::nullopt;
    }

    inline std::optional<wxString> openFile(wxWindow*       parent,
                                            const wxString& title  = _("Open file"),
                                            const wxString& filter = _("All files (*.*)|*.*"))
    {
        wxFileDialog dlg(parent, title, "", "", filter, wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetPath();
        return std::nullopt;
    }

    // ─────────────────────────────────────────────
    //  Progress — runs a task with a smooth progress dialog
    //
    //  Usage:
    //    wx::dialogs::progress(this, "Loading…", 100,
    //        [](wxProgressDialog& dlg, int i){
    //            dlg.Update(i);
    //            wxMilliSleep(30);
    //        });
    // ─────────────────────────────────────────────
    inline void progress(wxWindow*                                            parent,
                         const wxString&                                      title,
                         int                                                  steps,
                         std::function<void(wxProgressDialog&, int /*step*/)> perStep)
    {
        wxProgressDialog dlg(title, _("Please wait"), steps, parent,
                             wxPD_AUTO_HIDE | wxPD_SMOOTH | wxPD_APP_MODAL);
        for (int i = 0; i <= steps; ++i)
            perStep(dlg, i);
    }

} // namespace wx::widgets