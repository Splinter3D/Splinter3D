#include "application/tasks/task_manager.hpp"
#include "support/manual_poster.hpp"
#include "support/wx_helpers.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <latch>
#include <stdexcept>
#include <thread>
#include <wx/button.h>
#include <wx/gauge.h>
#include <wx/sizer.h>

using application::tasks::Task;
using application::tasks::TaskContext;
using application::tasks::TaskManager;
using tests::support::findDescendant;
using tests::support::ManualPoster;
using tests::support::pumpEvents;
using tests::support::ScopedFrame;
using ui::panels::TasksPanel;

namespace
{
    Task workTask(std::string name, std::function<void(TaskContext&)> work)
    {
        Task task;
        task.name      = std::move(name);
        task.operation = "test.ui";
        task.writes    = {1};
        task.work      = std::move(work);
        task.apply     = [] { };
        return task;
    }

    void click(wxButton* button)
    {
        wxCommandEvent event(wxEVT_BUTTON, button->GetId());
        event.SetEventObject(button);
        button->ProcessWindowEvent(event);
        pumpEvents(); // the panel acts on the click after the event returns
    }

    // Declared before the frame, so the manager outlives the panel listening to it.
    struct PanelFixture
    {
        ManualPoster  ui;
        TaskManager   manager{ui.poster()};
        ScopedFrame<> frame{nullptr, wxID_ANY, "TasksPanel test", wxDefaultPosition, wxSize(640, 480)};
        TasksPanel*   panel = nullptr;

        PanelFixture()
        {
            panel       = new TasksPanel(frame.get());
            auto* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(new wxPanel(frame.get()), 1, wxEXPAND);
            sizer->Add(panel, 0, wxEXPAND);
            frame->SetSizer(sizer);
            panel->attach(manager);
            frame->Show();
            pumpEvents();
        }

        ~PanelFixture()
        {
            manager.cancelAllAndWait();
        }

        bool drainUntil(const std::function<bool()>& condition)
        {
            return ui.drainUntil([&] {
                pumpEvents();
                return condition();
            });
        }
    };
} // namespace

TEST_CASE_METHOD(PanelFixture, "TasksPanel: hidden while there is no task", "[integration][ui][tasks]")
{
    CHECK(panel->cardCount() == 0);
    CHECK_FALSE(panel->IsShown());
}

TEST_CASE_METHOD(PanelFixture, "TasksPanel: a running task shows a card; Cancel removes it", "[integration][ui][tasks]")
{
    manager.submit(workTask("Import cube.stl", [](TaskContext& context) {
        while (!context.cancelled())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }));
    REQUIRE(panel->cardCount() == 1);
    CHECK(panel->IsShown());

    auto* button = findDescendant<wxButton>(panel);
    REQUIRE(button != nullptr);
    CHECK(button->GetLabel() == "Cancel");
    click(button);

    REQUIRE(drainUntil([&] { return manager.idle(); }));
    pumpEvents();
    CHECK(panel->cardCount() == 0);
    CHECK_FALSE(panel->IsShown());
}

TEST_CASE_METHOD(PanelFixture, "TasksPanel: a failed task offers Dismiss", "[integration][ui][tasks]")
{
    const auto id = manager.submit(workTask("Cut a - b", [](TaskContext&) { throw std::runtime_error("boom"); }));
    REQUIRE(drainUntil([&] { return manager.snapshot(id)->state == application::tasks::TaskState::Failed; }));

    auto* button = findDescendant<wxButton>(panel);
    REQUIRE(button != nullptr);
    CHECK(button->GetLabel() == "Dismiss");
    click(button);
    CHECK(panel->cardCount() == 0);
    CHECK_FALSE(panel->IsShown());
}

TEST_CASE_METHOD(PanelFixture, "TasksPanel: progress moves the gauge", "[integration][ui][tasks]")
{
    std::latch release(1);
    manager.submit(workTask("Export out.stl", [&](TaskContext& context) {
        context.setProgress(0.5);
        release.wait();
    }));

    auto* gauge = findDescendant<wxGauge>(panel);
    REQUIRE(gauge != nullptr);
    CHECK(drainUntil([&] { return gauge->GetValue() == 50; }));
    release.count_down();
    REQUIRE(drainUntil([&] { return manager.idle(); }));
}

TEST_CASE_METHOD(PanelFixture, "TasksPanel: progress dropped by the throttle still shows", "[integration][ui][tasks]")
{
    std::latch shown(1);
    std::latch release(1);
    manager.submit(workTask("Import big.stl", [&](TaskContext& context) {
        context.setProgress(0.3);
        shown.wait();
        context.setProgress(0.6); // right after the last post: dropped by the throttle
        release.wait();
    }));

    auto* gauge = findDescendant<wxGauge>(panel);
    REQUIRE(gauge != nullptr);
    REQUIRE(drainUntil([&] { return gauge->GetValue() == 30; }));
    shown.count_down();
    CHECK(drainUntil([&] { return gauge->GetValue() == 60; }));
    release.count_down();
    REQUIRE(drainUntil([&] { return manager.idle(); }));
}
