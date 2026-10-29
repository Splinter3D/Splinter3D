#include "model_management_dialog.hpp"

#include "ui/icons/Icons.hpp"

#include <algorithm>
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
                               wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_NO_HEADER);
        list_->InsertColumn(0, "Model", wxLIST_FORMAT_LEFT, 320);
        list_->InsertColumn(1, "", wxLIST_FORMAT_CENTER, ui::icons::svgIconColWidth); // eye
        list_->InsertColumn(2, "", wxLIST_FORMAT_CENTER, ui::icons::svgIconColWidth); // trash
        list_->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { ResizeColumns(); e.Skip(); });

        list_->AssignImageList(ui::icons::CreateImageList(), wxIMAGE_LIST_SMALL);

        list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ModelManagementDialog::OnItemActivated, this);
        list_->Bind(wxEVT_LEFT_DOWN, &ModelManagementDialog::OnListLeftDown, this);

        auto* close = new wxButton(this, wxID_CLOSE, "Close");
        close->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Hide(); });

        auto* buttons = new wxBoxSizer(wxHORIZONTAL);
        buttons->AddStretchSpacer();
        buttons->Add(close);

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->Add(list_, 1, wxEXPAND | wxALL, 12);
        layout->Add(buttons, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        SetSizerAndFit(layout);
        RefreshModelRows();
        CallAfter([this] { ResizeColumns(); });
    }

    void ModelManagementDialog::OnListLeftDown(wxMouseEvent& event)
    {
        // Get the selected row
        int        flags = 0;
        const long row   = list_->HitTest(event.GetPosition(), flags);
        // Click outside
        if (row == wxNOT_FOUND)
        {
            event.Skip();
            return;
        }

        // Get the selected column
        wxRect rect;
        list_->GetItemRect(row, rect, wxLIST_RECT_BOUNDS);
        const int x      = event.GetX() - rect.x;
        int       column = -1;
        int       left   = 0;
        for (int c = 0; c < list_->GetColumnCount(); ++c)
        {
            const int right = left + list_->GetColumnWidth(c);
            if (x >= left && x < right)
            {
                column = c;
                break;
            }
            left = right;
        }

        // Click on the visibility icon
        if (column == 1)
        {
            const auto visibility = visibility_();
            const bool visible    = static_cast<std::size_t>(row) < visibility.size() &&
                                    visibility[static_cast<std::size_t>(row)];
            set_visibility_(static_cast<int>(row), !visible);
            RefreshModelRows();
            return;
        }
        // Click on the delete icon
        if (column == 2)
        {
            remove_(static_cast<int>(row));
            RefreshModelRows();
            return;
        }

        event.Skip(); // normal selection / double-click behaviour
    }

    void ModelManagementDialog::RefreshModelRows()
    {
        list_->DeleteAllItems();
        const auto names      = names_();
        const auto visibility = visibility_();
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            const long row = list_->InsertItem(static_cast<long>(index), names[index]);
            list_->SetItemColumnImage(row, 0, -1);
            const bool visible = index < visibility.size() && visibility[index];
            list_->SetItemColumnImage(row, 1, visible ? ui::icons::IconEye : ui::icons::IconEyeOff);
            list_->SetItemColumnImage(row, 2, ui::icons::IconTrash);
        }
    }

    void ModelManagementDialog::OnItemActivated(wxListEvent& event)
    {
        focus_(static_cast<int>(event.GetIndex()));
    }

    void ModelManagementDialog::ResizeColumns()
    {
        const int total     = list_->GetClientSize().GetWidth();
        const int nameWidth = std::max(100, total - 2 * ui::icons::svgIconColWidth - 4); // -4: safety margin
        list_->SetColumnWidth(0, nameWidth);
    }
} // namespace ui::dialogs