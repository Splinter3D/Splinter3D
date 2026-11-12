#pragma once

#include <vector>
#include <wx/wx.h>

class wxToggleButton;

namespace ui::panels
{
    /**
     * @brief Right-hand dock: a strip of buttons, one per page, and the open
     * page (at most one) next to it.
     *
     * A page's button opens it (manually) or closes it. A page can also ask
     * to be shown (RequestOpen) and later say it no longer needs to be
     * (ReleaseOpen): the dock then opens and closes it by itself, unless the
     * user opened it, in which case it stays open until closed by hand. Once
     * the user has closed the dock by hand, requests no longer open it.
     * Opening or closing re-lays out the parent so its neighbours take the
     * space back.
     */
    class SideDock : public wxPanel
    {
      public:
        explicit SideDock(wxWindow* parent);

        /**
         * @brief Adds a page (a child of this dock) and its strip button: icon
         * with label as tooltip, or label alone without icon. The page starts closed.
         */
        void AddPage(wxWindow* page, const wxBitmap& icon, const wxString& label);

        /** @brief Opens page as the user would (it stays open until closed by hand). */
        void Open(wxWindow* page);
        /** @brief Closes the dock as the user would (requests no longer open it). */
        void Close();

        /** @brief Opens page unless the dock already shows a page or the user closed it. */
        void RequestOpen(wxWindow* page);
        /** @brief Closes page if RequestOpen opened it and the user did not take it over since. */
        void ReleaseOpen(wxWindow* page);

        /** @brief The open page, or nullptr. */
        [[nodiscard]] wxWindow* CurrentPage() const noexcept;
        [[nodiscard]] bool      IsOpenedManually() const noexcept;
        /** @brief page's strip button, or nullptr. */
        [[nodiscard]] wxToggleButton* GetButton(wxWindow* page) const;

      private:
        struct Page
        {
            wxWindow*       window = nullptr;
            wxToggleButton* button = nullptr;
        };

        void show(wxWindow* page, bool manual);
        void onButton(wxWindow* page);

        std::vector<Page> pages_;
        wxBoxSizer*       content_        = nullptr;
        wxBoxSizer*       strip_          = nullptr;
        wxWindow*         current_        = nullptr;
        bool              manual_         = false;
        bool              closed_by_hand_ = false;
    };
} // namespace ui::panels
