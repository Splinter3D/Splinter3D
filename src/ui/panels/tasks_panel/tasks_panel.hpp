#pragma once

#include "application/tasks/task.hpp"

#include <cstddef>
#include <functional>
#include <vector>
#include <wx/wx.h>

namespace application::tasks
{
    class TaskManager;
} // namespace application::tasks

class wxGauge;

namespace ui::panels
{
    /**
     * @brief Lists background tasks as cards: name, progress bar, status,
     * and Cancel (queued/running) or Dismiss (failed).
     *
     * Says when its first card appears and its last one goes (see
     * SetOnBusyChanged) so its container can show and hide it; shows a
     * placeholder while empty. Driven by a TaskManager's listener, on the UI
     * thread.
     */
    class TasksPanel : public wxPanel
    {
      public:
        explicit TasksPanel(wxWindow* parent);

        /** @brief Follows tasks from now on (replacing its listener); tasks must outlive the panel's use of it. */
        void attach(application::tasks::TaskManager& tasks);

        /** @brief Called with true when the first card appears, false when the last one goes. */
        void SetOnBusyChanged(std::function<void(bool busy)> callback);

        [[nodiscard]] std::size_t cardCount() const noexcept;

#if defined(SPLINTER3D_TESTING)
        /** @brief Test-only: what the refresh timer does (tests don't run wx's event loop, so timers may not fire). */
        void RefreshRunningCards();
#endif

      private:
        struct Card
        {
            application::tasks::TaskId    id     = 0;
            application::tasks::TaskState state  = application::tasks::TaskState::Queued;
            wxPanel*                      panel  = nullptr;
            wxGauge*                      gauge  = nullptr;
            wxStaticText*                 status = nullptr;
            wxButton*                     button = nullptr;
        };

        void  onTask(const application::tasks::TaskSnapshot& snapshot, bool removed);
        Card& addCard(const application::tasks::TaskSnapshot& snapshot);
        void  updateCard(Card& card, const application::tasks::TaskSnapshot& snapshot);
        void  removeCard(application::tasks::TaskId id);
        void  onButton(application::tasks::TaskId id);
        void  onPulse(wxTimerEvent& event);
        // Re-reads running cards' progress (animates the indeterminate ones);
        // false when none is running any more.
        bool refreshRunningCards();
        void refreshLayout();

        application::tasks::TaskManager* tasks_ = nullptr;
        std::vector<Card>                cards_; // submission order
        wxBoxSizer*                      list_  = nullptr;
        wxStaticText*                    empty_ = nullptr;
        wxTimer                          pulse_;
        std::function<void(bool busy)>   on_busy_changed_;
    };
} // namespace ui::panels
