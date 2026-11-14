#include "side_dock.hpp"

#include <algorithm>
#include <wx/statline.h>
#include <wx/tglbtn.h>

namespace ui::panels
{
    SideDock::SideDock(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
    {
        auto* root = new wxBoxSizer(wxHORIZONTAL);
        content_   = new wxBoxSizer(wxVERTICAL);
        strip_     = new wxBoxSizer(wxVERTICAL);
        root->Add(content_, 0, wxEXPAND);
        root->Add(new wxStaticLine(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL), 0, wxEXPAND);
        root->Add(strip_, 0, wxEXPAND | wxALL, 4);
        SetSizer(root);
    }

    void SideDock::AddPage(wxWindow* page, const wxBitmap& icon, const wxString& label)
    {
        if (page == nullptr)
            return;
        page->Hide();
        content_->Add(page, 1, wxEXPAND);

        // Icon only, the label as tooltip; the label shows when there is no icon.
        auto* button = new wxToggleButton(this, wxID_ANY, icon.IsOk() ? wxString() : label, wxDefaultPosition,
                                          wxDefaultSize, wxBU_EXACTFIT);
        if (icon.IsOk())
            button->SetBitmap(icon);
        button->SetToolTip(label);
        // Showing or hiding a page from inside the click handler re-lays out
        // the window under the click; act once it returns.
        button->Bind(wxEVT_TOGGLEBUTTON, [this, page](wxCommandEvent&) { CallAfter([this, page] { onButton(page); }); });
        strip_->Add(button, 0, wxEXPAND | wxBOTTOM, 4);
        pages_.push_back({page, button});
    }

    void SideDock::Open(wxWindow* page)
    {
        show(page, true);
    }

    void SideDock::Close()
    {
        closed_by_hand_ = true;
        show(nullptr, false);
    }

    void SideDock::RequestOpen(wxWindow* page)
    {
        if (current_ == nullptr && !closed_by_hand_)
            show(page, false);
    }

    void SideDock::ReleaseOpen(wxWindow* page)
    {
        if (current_ == page && !manual_)
            show(nullptr, false);
    }

    wxWindow* SideDock::CurrentPage() const noexcept
    {
        return current_;
    }

    bool SideDock::IsOpenedManually() const noexcept
    {
        return current_ != nullptr && manual_;
    }

    bool SideDock::IsClosedByHand() const noexcept
    {
        return closed_by_hand_;
    }

    wxToggleButton* SideDock::GetButton(wxWindow* page) const
    {
        const auto found = std::find_if(pages_.begin(), pages_.end(), [page](const Page& p) { return p.window == page; });
        return found != pages_.end() ? found->button : nullptr;
    }

    void SideDock::onButton(wxWindow* page)
    {
        if (current_ == page)
            Close();
        else
            Open(page);
    }

    void SideDock::show(wxWindow* page, bool manual)
    {
        if (page != nullptr && GetButton(page) == nullptr)
            return;
        const bool changed = current_ != page;
        current_           = page;
        manual_            = page != nullptr && manual;
        // Also resyncs a button the click toggled when nothing changed.
        for (const auto& p : pages_)
        {
            p.window->Show(p.window == current_);
            p.button->SetValue(p.window == current_);
        }
        if (!changed)
            return;
        Layout();
        if (GetParent() != nullptr)
            GetParent()->Layout();
    }
} // namespace ui::panels
