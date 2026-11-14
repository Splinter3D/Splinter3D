#include "models_panel.hpp"

#include "ui/framework/wx/locale/locale.hpp"
#include "ui/icons/Icons.hpp"

#include <algorithm>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/wrapsizer.h>

using ui::framework::wx::locale::tr;

namespace ui::panels
{
    namespace
    {
        constexpr int kWidth = 260;
    } // namespace

    ModelsPanel::ModelsPanel(wxWindow* parent, Actions actions)
        : wxPanel(parent, wxID_ANY), actions_(std::move(actions))
    {
        SetMinSize(wxSize(kWidth, -1));

        auto* title = new wxStaticText(this, wxID_ANY, tr("models.title"));
        title->SetFont(title->GetFont().Bold());

        list_ = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT | wxLC_NO_HEADER);
        list_->InsertColumn(0, tr("models.column"), wxLIST_FORMAT_LEFT, kWidth);
        list_->InsertColumn(1, "", wxLIST_FORMAT_CENTER, ui::icons::svgIconColWidth); // eye
        list_->InsertColumn(2, "", wxLIST_FORMAT_CENTER, ui::icons::svgIconColWidth); // trash
        list_->Bind(wxEVT_SIZE, [this](wxSizeEvent& e) { ResizeColumns(); e.Skip(); });

        list_->SetSmallImages(ui::icons::ListIcons());

        list_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &ModelsPanel::OnItemActivated, this);
        list_->Bind(wxEVT_LEFT_DOWN, &ModelsPanel::OnListLeftDown, this);
        list_->Bind(wxEVT_LEFT_DCLICK, &ModelsPanel::OnListDoubleClick, this);

        // Wraps: a long translation goes to the next line instead of widening the dock.
        auto*      buttons   = new wxWrapSizer(wxHORIZONTAL);
        const auto addButton = [&](const char* label, const char* tooltip, std::function<void(std::vector<int>)> Actions::* action) {
            auto* button = new wxButton(this, wxID_ANY, tr(label), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
            button->SetToolTip(tr(tooltip));
            button->Bind(wxEVT_BUTTON, [this, action](wxCommandEvent&) {
                const auto selected = actions_.selected();
                if (!selected.empty() && actions_.*action)
                    (actions_.*action)(selected);
            });
            buttons->Add(button, 0, wxRIGHT | wxBOTTOM, 4);
            selection_buttons_.push_back(button);
        };
        addButton("models.hide", "models.hide_tooltip", &Actions::hide);
        addButton("models.delete", "models.delete_tooltip", &Actions::removeMany);
        addButton("models.show_only", "models.show_only_tooltip", &Actions::showOnly);
        addButton("models.duplicate", "models.duplicate_tooltip", &Actions::duplicate);
        auto* showAll = new wxButton(this, wxID_ANY, tr("models.show_all"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        showAll->SetToolTip(tr("models.show_all_tooltip"));
        showAll->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
            if (actions_.showAll)
                actions_.showAll();
        });
        buttons->Add(showAll, 0, wxRIGHT | wxBOTTOM, 4);

        auto* includeHidden = new wxCheckBox(this, wxID_ANY, tr("models.include_hidden"));
        includeHidden->SetToolTip(tr("models.include_hidden_tooltip"));
        includeHidden->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& event) { include_hidden_ = event.IsChecked(); });

        auto* layout = new wxBoxSizer(wxVERTICAL);
        layout->Add(title, 0, wxALL, 8);
        layout->Add(list_, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
        layout->Add(includeHidden, 0, wxLEFT | wxRIGHT | wxTOP, 8);
        layout->Add(buttons, 0, wxEXPAND | wxALL, 8);
        SetSizer(layout);
        RefreshModelRows();
        CallAfter([this] { ResizeColumns(); });
    }

    void ModelsPanel::OnListLeftDown(wxMouseEvent& event)
    {
        // Get the selected row
        int        flags = 0;
        const long row   = list_->HitTest(event.GetPosition(), flags);
        list_->SetFocus();
        // Click below the rows
        if (row == wxNOT_FOUND)
        {
            clickName(-1, event);
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
            const auto visibility = actions_.visibility();
            const bool visible    = static_cast<std::size_t>(row) < visibility.size() &&
                                    visibility[static_cast<std::size_t>(row)];
            actions_.setVisibility(static_cast<int>(row), !visible);
            RefreshModelRows();
            return;
        }
        // Click on the delete icon
        if (column == 2)
        {
            actions_.remove(static_cast<int>(row));
            RefreshModelRows();
            return;
        }

        // Not skipped: the list's own selection would differ from the viewer's.
        clickName(static_cast<int>(row), event);
    }

    void ModelsPanel::clickName(int row, const wxMouseEvent& event)
    {
        if (row < 0)
        {
            if (!event.CmdDown() && !event.ShiftDown())
                actions_.select({});
            return;
        }
        if (event.ShiftDown() && anchor_ >= 0 && anchor_ < list_->GetItemCount())
        {
            const auto       visibility = actions_.visibility();
            std::vector<int> range;
            for (int r = std::min(anchor_, row); r <= std::max(anchor_, row); ++r)
                if (include_hidden_ || (static_cast<std::size_t>(r) < visibility.size() && visibility[static_cast<std::size_t>(r)]))
                    range.push_back(r);
            actions_.select(std::move(range)); // the anchor stays, for the next Shift-click
            return;
        }
        anchor_ = row;
        if (event.CmdDown())
            actions_.toggleSelected(row);
        else
            actions_.select({row});
    }

    void ModelsPanel::OnListDoubleClick(wxMouseEvent& event)
    {
        // The list's own activation needs the clicks OnListLeftDown keeps.
        int        flags = 0;
        const long row   = list_->HitTest(event.GetPosition(), flags);
        if (row != wxNOT_FOUND)
            actions_.focus(static_cast<int>(row));
    }

    void ModelsPanel::ShowSelection()
    {
        const auto selected = actions_.selected();
        for (auto* button : selection_buttons_)
            button->Enable(!selected.empty());
        for (int row = 0; row < list_->GetItemCount(); ++row)
        {
            const bool on = std::find(selected.begin(), selected.end(), row) != selected.end();
            list_->SetItemState(row, on ? wxLIST_STATE_SELECTED : 0, wxLIST_STATE_SELECTED);
        }
    }

    void ModelsPanel::RefreshModelRows()
    {
        list_->DeleteAllItems();
        const auto names      = actions_.names();
        const auto visibility = actions_.visibility();
        for (std::size_t index = 0; index < names.size(); ++index)
        {
            const long row = list_->InsertItem(static_cast<long>(index), names[index]);
            list_->SetItemColumnImage(row, 0, -1);
            const bool visible = index < visibility.size() && visibility[index];
            list_->SetItemColumnImage(row, 1, visible ? ui::icons::IconEye : ui::icons::IconEyeOff);
            list_->SetItemColumnImage(row, 2, ui::icons::IconTrash);
        }
        if (anchor_ >= list_->GetItemCount())
            anchor_ = -1;
        ShowSelection();
    }

    void ModelsPanel::OnItemActivated(wxListEvent& event)
    {
        actions_.focus(static_cast<int>(event.GetIndex()));
    }

    void ModelsPanel::ResizeColumns()
    {
        const int total     = list_->GetClientSize().GetWidth();
        const int nameWidth = std::max(100, total - 2 * ui::icons::svgIconColWidth - 4); // -4: safety margin
        list_->SetColumnWidth(0, nameWidth);
    }
} // namespace ui::panels
