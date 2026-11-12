#include "tasks_panel.hpp"

#include "application/tasks/task_manager.hpp"
#include "ui/framework/wx/locale/locale.hpp"

#include <algorithm>
#include <utility>
#include <wx/gauge.h>
#include <wx/settings.h>

using ui::framework::wx::locale::tr;

namespace ui::panels
{
    namespace
    {
        constexpr int kWidth           = 260;
        constexpr int kGaugeRange      = 100;
        constexpr int kPulseIntervalMs = 100;

        using application::tasks::TaskState;
    } // namespace

    TasksPanel::TasksPanel(wxWindow* parent)
        : wxPanel(parent, wxID_ANY)
        , pulse_(this)
    {
        SetMinSize(wxSize(kWidth, -1));

        auto* root  = new wxBoxSizer(wxVERTICAL);
        auto* title = new wxStaticText(this, wxID_ANY, tr("tasks.title"));
        title->SetFont(title->GetFont().Bold());
        root->Add(title, 0, wxALL, 8);

        empty_ = new wxStaticText(this, wxID_ANY, tr("tasks.empty"));
        empty_->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_GRAYTEXT));
        root->Add(empty_, 0, wxLEFT | wxRIGHT, 8);

        list_ = new wxBoxSizer(wxVERTICAL);
        root->Add(list_, 0, wxEXPAND | wxLEFT | wxRIGHT, 8);
        SetSizer(root);

        Bind(wxEVT_TIMER, &TasksPanel::onPulse, this, pulse_.GetId());
    }

    void TasksPanel::SetOnBusyChanged(std::function<void(bool busy)> callback)
    {
        on_busy_changed_ = std::move(callback);
    }

    void TasksPanel::attach(application::tasks::TaskManager& tasks)
    {
        tasks_ = &tasks;
        tasks_->setListener(
            [this](const application::tasks::TaskSnapshot& snapshot, bool removed) { onTask(snapshot, removed); });
        for (const auto& snapshot : tasks_->snapshots())
            onTask(snapshot, false);
    }

    std::size_t TasksPanel::cardCount() const noexcept
    {
        return cards_.size();
    }

    void TasksPanel::onTask(const application::tasks::TaskSnapshot& snapshot, bool removed)
    {
        if (removed)
        {
            removeCard(snapshot.id);
            return;
        }
        const auto found =
            std::find_if(cards_.begin(), cards_.end(), [&](const Card& card) { return card.id == snapshot.id; });
        updateCard(found != cards_.end() ? *found : addCard(snapshot), snapshot);
        refreshLayout();
    }

    TasksPanel::Card& TasksPanel::addCard(const application::tasks::TaskSnapshot& snapshot)
    {
        Card card;
        card.id    = snapshot.id;
        card.panel = new wxPanel(this, wxID_ANY);

        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* row   = new wxBoxSizer(wxHORIZONTAL);
        auto* name  = new wxStaticText(card.panel, wxID_ANY, wxString::FromUTF8(snapshot.name), wxDefaultPosition,
                                       wxDefaultSize, wxST_ELLIPSIZE_END);
        name->SetFont(name->GetFont().Bold());
        card.button = new wxButton(card.panel, wxID_ANY, tr("tasks.cancel"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
        row->Add(name, 1, wxALIGN_CENTER_VERTICAL);
        row->Add(card.button, 0, wxLEFT, 6);

        card.gauge  = new wxGauge(card.panel, wxID_ANY, kGaugeRange, wxDefaultPosition, wxSize(-1, 8));
        card.status = new wxStaticText(card.panel, wxID_ANY, "");

        sizer->Add(row, 0, wxEXPAND);
        sizer->Add(card.gauge, 0, wxEXPAND | wxTOP | wxBOTTOM, 4);
        sizer->Add(card.status, 0, wxEXPAND);
        card.panel->SetSizer(sizer);

        // Acting inside the click handler could destroy the button that is
        // being clicked (cancel() removes the card); act once it returns.
        const application::tasks::TaskId id = snapshot.id;
        card.button->Bind(wxEVT_BUTTON, [this, id](wxCommandEvent&) { CallAfter([this, id] { onButton(id); }); });

        list_->Add(card.panel, 0, wxEXPAND | wxBOTTOM, 10);
        cards_.push_back(card);
        return cards_.back();
    }

    void TasksPanel::updateCard(Card& card, const application::tasks::TaskSnapshot& snapshot)
    {
        card.state = snapshot.state;
        switch (snapshot.state)
        {
            case TaskState::Queued:
                card.gauge->Show();
                card.gauge->SetValue(0);
                card.button->SetLabel(tr("tasks.cancel"));
                card.button->Show();
                card.status->SetLabel(wxString::FromUTF8(snapshot.status));
                break;
            case TaskState::Running:
                card.gauge->Show();
                if (snapshot.progress)
                    card.gauge->SetValue(static_cast<int>(*snapshot.progress * kGaugeRange + 0.5));
                card.button->SetLabel(tr("tasks.cancel"));
                card.button->Show();
                card.status->SetLabel(snapshot.progress ? wxString::Format("%d%%", card.gauge->GetValue())
                                                        : wxString::FromUTF8(snapshot.status));
                break;
            case TaskState::Cancelling:
                card.button->Hide();
                card.status->SetLabel(wxString::FromUTF8(snapshot.status));
                break;
            case TaskState::Failed:
                card.gauge->Hide();
                card.button->SetLabel(tr("tasks.dismiss"));
                card.button->Show();
                card.status->SetLabel(tr("tasks.failed", {{"error", wxString::FromUTF8(snapshot.error)}}));
                break;
            default:
                break;
        }
        card.status->Wrap(kWidth - 16);

        // Running cards are refreshed (and indeterminate bars animated) by
        // the timer: progress notifications are throttled and may skip the
        // last update before a long step.
        if (snapshot.state == TaskState::Running && !pulse_.IsRunning())
            pulse_.Start(kPulseIntervalMs);
        card.panel->Layout();
    }

    void TasksPanel::removeCard(application::tasks::TaskId id)
    {
        const auto found = std::find_if(cards_.begin(), cards_.end(), [id](const Card& card) { return card.id == id; });
        if (found == cards_.end())
            return;
        list_->Detach(found->panel);
        found->panel->Destroy();
        cards_.erase(found);
        refreshLayout();
    }

    void TasksPanel::onButton(application::tasks::TaskId id)
    {
        const auto found = std::find_if(cards_.begin(), cards_.end(), [id](const Card& card) { return card.id == id; });
        if (found == cards_.end() || tasks_ == nullptr)
            return;
        if (found->state == TaskState::Failed)
            tasks_->dismiss(id);
        else
            tasks_->cancel(id);
    }

    void TasksPanel::onPulse(wxTimerEvent&)
    {
        if (!refreshRunningCards())
            pulse_.Stop();
    }

#if defined(SPLINTER3D_TESTING)
    void TasksPanel::RefreshRunningCards()
    {
        (void) refreshRunningCards();
    }
#endif

    bool TasksPanel::refreshRunningCards()
    {
        bool running = false;
        if (tasks_ != nullptr)
        {
            for (auto& card : cards_)
            {
                if (card.state != TaskState::Running)
                    continue;
                const auto snapshot = tasks_->snapshot(card.id);
                if (!snapshot || snapshot->state != TaskState::Running)
                    continue;
                running = true;
                if (snapshot->progress)
                    updateCard(card, *snapshot);
                else
                    card.gauge->Pulse();
            }
        }
        return running;
    }

    void TasksPanel::refreshLayout()
    {
        const bool busy = !cards_.empty();
        if (empty_->IsShown() == busy)
        {
            empty_->Show(!busy);
            if (on_busy_changed_)
                on_busy_changed_(busy);
        }
        Layout();
    }
} // namespace ui::panels
