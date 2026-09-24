#include "model_management_dialog.hpp"

#include <wx/button.h>
#include <wx/sizer.h>

namespace ui::dialogs
{
    ModelManagementDialog::ModelManagementDialog(
        wxWindow*                              parent,
        std::function<std::vector<wxString>()> names,
        std::function<std::vector<bool>()>     visibility,
        std::function<void(int, bool)>         setVisibility,
        std::function<void(int)>               remove,
        std::function<void(int)>               focus)
        : FloatingMenu(parent, "Models", wxSize(420, 320)), names_(std::move(names)), visibility_(std::move(visibility)), set_visibility_(std::move(setVisibility)), remove_(std::move(remove)), focus_(std::move(focus))
    {
        list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT | wxLC_SINGLE_SEL);
        list_->InsertColumn(0, "Model", wxLIST_FORMAT_LEFT, 280);
        list_->InsertColumn(1, "Visible", wxLIST_FORMAT_LEFT, 80);
        list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ModelManagementDialog::OnItemActivated, this);

        auto* buttons      = new wxBoxSizer(wxHORIZONTAL);
        auto* toggle       = new wxButton(this, wxID_ANY, "Toggle visibility");
        auto* removeButton = new wxButton(this, wxID_DELETE, "Delete");
        auto* close        = new wxButton(this, wxID_CLOSE, "Close");
        toggle->Bind(wxEVT_BUTTON, &ModelManagementDialog::OnToggle, this);
        removeButton->Bind(wxEVT_BUTTON, &ModelManagementDialog::OnDelete, this);
        close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Hide(); });
        buttons->Add(toggle, 0, wxRIGHT, 8);
        buttons->Add(removeButton, 0, wxRIGHT, 8);
        buttons->AddStretchSpacer();
        buttons->Add(close);

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->Add(list_, 1, wxEXPAND | wxALL, 12);
        layout->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
        SetSizerAndFit(layout);
        RefreshModels();
    }

    void ModelManagementDialog::RefreshModels()
    {
        list_->DeleteAllItems();
        const auto names      = names_();
        const auto visibility = visibility_();
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            const long row = list_->InsertItem(static_cast<long>(index), names[index]);
            list_->SetItem(row, 1, index < visibility.size() && visibility[index] ? "Yes" : "No");
        }
    }

    void ModelManagementDialog::OnItemActivated(wxListEvent& event)
    {
        focus_(static_cast<int>(event.GetIndex()));
    }

    void ModelManagementDialog::OnToggle(wxCommandEvent& event)
    {
        (void) event;
        const long selected = list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected < 0)
            return;

        const auto visibility = visibility_();
        const bool visible    = static_cast<std::size_t>(selected) < visibility.size() &&
                                visibility[static_cast<std::size_t>(selected)];
        set_visibility_(static_cast<int>(selected), !visible);
        RefreshModels();
    }

    void ModelManagementDialog::OnDelete(wxCommandEvent& event)
    {
        (void) event;
        const long selected = list_->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (selected < 0)
            return;

        remove_(static_cast<int>(selected));
        RefreshModels();
    }
} // namespace ui::dialogs