#pragma once
#include <functional>
#include <optional>

#include <wx/colordlg.h>
#include <wx/filedlg.h>
#include <wx/numdlg.h>
#include <wx/progdlg.h>
#include <wx/textdlg.h>
#include <wx/wx.h>

namespace ui::dialogs
{
    /**
     * @brief Displays an information message box.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * ui::dialogs::info(this, "File saved successfully!");
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param msg Message displayed inside the dialog.
     * @param title Dialog title.
     */
    inline void info(wxWindow *parent,
                     const wxString &msg,
                     const wxString &title = _("Info"))
    {
        wxMessageBox(
            msg,
            title,
            wxOK | wxICON_INFORMATION,
            parent);
    }

    /**
     * @brief Displays a warning message box.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * ui::dialogs::warning(this, "This action may have consequences.");
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param msg Message displayed inside the dialog.
     * @param title Dialog title.
     */
    inline void warning(wxWindow *parent,
                        const wxString &msg,
                        const wxString &title = _("Warning"))
    {
        wxMessageBox(
            msg,
            title,
            wxOK | wxICON_WARNING,
            parent);
    }

    /**
     * @brief Displays an error message box.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * ui::dialogs::error(this, "Unable to open file.");
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param msg Message displayed inside the dialog.
     * @param title Dialog title.
     */
    inline void error(wxWindow *parent,
                      const wxString &msg,
                      const wxString &title = _("Error"))
    {
        wxMessageBox(
            msg,
            title,
            wxOK | wxICON_ERROR,
            parent);
    }

    /**
     * @brief Displays a confirmation dialog.
     *
     * Returns true when the user confirms the action.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * if (ui::dialogs::confirm(this, "Delete this file?"))
     * {
     *     deleteFile();
     * }
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param msg Message displayed inside the dialog.
     * @param title Dialog title.
     *
     * @return true if the user clicked "Yes".
     */
    inline bool confirm(wxWindow *parent,
                        const wxString &msg,
                        const wxString &title = _("Question"))
    {
        return wxMessageBox(
                   msg,
                   title,
                   wxYES_NO | wxICON_QUESTION,
                   parent) == wxYES;
    }

    /**
     * @brief Opens a text input dialog.
     *
     * Returns an empty optional if the user cancels the dialog.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto name = ui::dialogs::askText(
     *     this,
     *     "Enter your name:");
     *
     * if (name)
     *     wxLogMessage(*name);
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param prompt Text displayed above the input field.
     * @param title Dialog title.
     * @param defaultValue Initial value.
     *
     * @return Entered text, or std::nullopt if cancelled.
     */
    inline std::optional<wxString> askText(
        wxWindow *parent,
        const wxString &prompt,
        const wxString &title = _("Input"),
        const wxString &defaultValue = "")
    {
        wxTextEntryDialog dlg(
            parent,
            prompt,
            title,
            defaultValue);

        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetValue();

        return std::nullopt;
    }

    /**
     * @brief Opens an integer input dialog.
     *
     * Returns an empty optional if the user cancels the dialog.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto amount = ui::dialogs::askInteger(
     *     this,
     *     "Select amount:",
     *     "Input",
     *     10,
     *     0,
     *     100);
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param prompt Text displayed above the input field.
     * @param title Dialog title.
     * @param defaultValue Initial value.
     * @param min Minimum allowed value.
     * @param max Maximum allowed value.
     *
     * @return Entered integer, or std::nullopt if cancelled.
     */
    inline std::optional<long> askInteger(
        wxWindow *parent,
        const wxString &prompt,
        const wxString &title = _("Input"),
        long defaultValue = 0,
        long min = 0,
        long max = 1000)
    {
        wxNumberEntryDialog dlg(
            parent,
            prompt,
            _("Number:"),
            title,
            defaultValue,
            min,
            max);

        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetValue();

        return std::nullopt;
    }

    /**
     * @brief Opens a color selection dialog.
     *
     * Returns an empty optional if the user cancels the dialog.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto color = ui::dialogs::pickColor(this);
     *
     * if (color)
     *     panel->SetBackgroundColour(*color);
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     *
     * @return Selected color, or std::nullopt if cancelled.
     */
    inline std::optional<wxColour> pickColor(wxWindow *parent)
    {
        wxColourDialog dlg(parent);

        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetColourData().GetColour();

        return std::nullopt;
    }

    /**
     * @brief Opens a file selection dialog.
     *
     * Returns an empty optional if the user cancels the dialog.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto file = ui::dialogs::openFile(this);
     *
     * if (file)
     *     loadFile(*file);
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param title Dialog title.
     * @param filter File filter.
     *
     * @return Selected file path, or std::nullopt if cancelled.
     */
    inline std::optional<wxString> openFile(
        wxWindow *parent,
        const wxString &title = _("Open file"),
        const wxString &filter = _("All files (*.*)|*.*"))
    {
        wxFileDialog dlg(
            parent,
            title,
            "",
            "",
            filter,
            wxFD_OPEN | wxFD_FILE_MUST_EXIST);

        if (dlg.ShowModal() == wxID_OK)
            return dlg.GetPath();

        return std::nullopt;
    }

    /**
     * @brief Runs a task while displaying a progress dialog.
     *
     * The callback is executed once per step and receives the progress dialog
     * and the current step index.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * ui::dialogs::progress(
     *     this,
     *     "Loading...",
     *     100,
     *     [](wxProgressDialog& dlg, int step)
     *     {
     *         dlg.Update(step);
     *         wxMilliSleep(20);
     *     });
     * @endcode
     *
     * @param parent Parent window owning the dialog.
     * @param title Dialog title.
     * @param steps Maximum progress value.
     * @param perStep Function executed for each progress step.
     */
    inline void progress(
        wxWindow *parent,
        const wxString &title,
        int steps,
        std::function<void(wxProgressDialog &, int)> perStep)
    {
        wxProgressDialog dlg(
            title,
            _("Please wait"),
            steps,
            parent,
            wxPD_AUTO_HIDE |
                wxPD_SMOOTH |
                wxPD_APP_MODAL);

        for (int i = 0; i <= steps; ++i)
            perStep(dlg, i);
    }

} // namespace ui::dialogs
