#include "application/tasks/task_manager.hpp"
#include "platform/threading/dispatcher.hpp"
#include "support/manual_poster.hpp"

#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <functional>
#include <latch>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using application::tasks::Task;
using application::tasks::TaskContext;
using application::tasks::TaskId;
using application::tasks::TaskManager;
using application::tasks::TaskSnapshot;
using application::tasks::TaskState;
using platform::threading::Dispatcher;
using tests::support::ManualPoster;

namespace
{
    using namespace std::chrono_literals;

    // Polls (without draining) until condition() holds.
    bool waitFor(const std::function<bool()>& condition, std::chrono::milliseconds timeout = 5s)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (!condition() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(1ms);
        return condition();
    }

    // Makes `operation` known as fully serial, so the Dispatcher gives it one worker.
    void serial(const std::string& operation)
    {
        platform::benchmark::Report report;
        report.time     = platform::benchmark::Duration(100.0);
        report.cpuTime  = platform::benchmark::Duration(100.0);
        report.cpuCores = 1.0;
        platform::threading::ScalabilityProfile profile;
        profile.record(4, report, false);
        Dispatcher::getInstance().setProfile(operation, profile);
    }

    Task writeTask(std::string name, std::vector<application::service::ModelId> writes,
                   std::function<void(TaskContext&)> work = {}, std::function<void()> apply = {})
    {
        Task task;
        task.name      = std::move(name);
        task.operation = work ? "test.work" : "";
        task.writes    = std::move(writes);
        task.work      = std::move(work);
        task.apply     = apply ? std::move(apply) : [] { };
        return task;
    }

    Task readTask(std::string name, std::vector<application::service::ModelId> reads,
                  std::function<void(TaskContext&)> work, std::function<void()> prepared = {})
    {
        Task task;
        task.name      = std::move(name);
        task.operation = "test.read";
        task.reads     = std::move(reads);
        task.prepare   = [prepared]() -> std::optional<std::string> {
            if (prepared)
                prepared();
            return std::nullopt;
        };
        task.work  = std::move(work);
        task.apply = [] { };
        return task;
    }

    std::optional<TaskState> stateOf(const TaskManager& tasks, TaskId id)
    {
        const auto snapshot = tasks.snapshot(id);
        return snapshot ? std::optional<TaskState>(snapshot->state) : std::nullopt;
    }
} // namespace

TEST_CASE("TaskManager: tasks on the same model apply in submission order", "[application][tasks]")
{
    ManualPoster     ui;
    TaskManager      tasks(ui.poster());
    std::latch       release(1);
    std::vector<int> order;

    const TaskId first  = tasks.submit(writeTask("first", {1}, [&](TaskContext&) { release.wait(); }, [&] { order.push_back(1); }));
    const TaskId second = tasks.submit(writeTask("second", {1}, [](TaskContext&) { }, [&] { order.push_back(2); }));

    CHECK(stateOf(tasks, first) == TaskState::Running);
    REQUIRE(stateOf(tasks, second) == TaskState::Queued);
    CHECK(tasks.snapshot(second)->status == "Queued · waiting for \"first\"");

    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(order == std::vector<int>{1, 2});
}

TEST_CASE("TaskManager: tasks on different models run in parallel", "[application][tasks]")
{
    if (Dispatcher::getInstance().total() < 3)
        SKIP("needs at least three cores");
    serial("test.work");

    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   bothInside(2);
    std::latch   release(1);
    auto         work = [&](TaskContext&) {
        bothInside.count_down();
        release.wait();
    };
    const TaskId a = tasks.submit(writeTask("a", {1}, work));
    const TaskId b = tasks.submit(writeTask("b", {2}, work));

    REQUIRE(ui.drainUntil([&] { return bothInside.try_wait(); })); // b launches once a holds its worker
    CHECK(stateOf(tasks, a) == TaskState::Running);
    CHECK(stateOf(tasks, b) == TaskState::Running);
    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
}

TEST_CASE("TaskManager: a read does not block a later write", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   release(1);
    tasks.submit(readTask("export", {1}, [&](TaskContext&) { release.wait(); }));

    bool moved = false;
    tasks.submit(writeTask("move", {1}, {}, [&] { moved = true; }));
    CHECK(moved);

    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
}

TEST_CASE("TaskManager: a write blocks a later read until applied", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   release(1);
    bool         applied     = false;
    bool         readSawEdit = false;
    tasks.submit(writeTask("cut", {1}, [&](TaskContext&) { release.wait(); }, [&] { applied = true; }));
    const TaskId read =
        tasks.submit(readTask("export", {1}, [](TaskContext&) { }, [&] { readSawEdit = applied; }));

    REQUIRE(stateOf(tasks, read) == TaskState::Queued);
    CHECK(tasks.snapshot(read)->status == "Queued · waiting for \"cut\"");
    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(readSawEdit);
}

TEST_CASE("TaskManager: an instant edit on a free model applies inside submit, without a card", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    int          notifications = 0;
    tasks.setListener([&](const TaskSnapshot&, bool) { ++notifications; });

    bool applied = false;
    tasks.submit(writeTask("move", {1}, {}, [&] { applied = true; }));
    CHECK(applied);
    CHECK(notifications == 0);
    CHECK(tasks.idle());
}

TEST_CASE("TaskManager: a task queued for workers starts once they free up", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   inside(1);
    std::latch   release(1);

    // Unknown operation alone: it takes every worker.
    Task hog      = writeTask("hog", {1}, [&](TaskContext&) {
        inside.count_down();
        release.wait();
    });
    hog.operation = "test.hog";
    tasks.submit(std::move(hog));
    inside.wait();
    REQUIRE(Dispatcher::getInstance().freeWorkers() == 0);

    bool         ran   = false;
    const TaskId other = tasks.submit(writeTask("other", {2}, [&](TaskContext&) { ran = true; }));
    REQUIRE(stateOf(tasks, other) == TaskState::Queued);
    CHECK(tasks.snapshot(other)->status == "Queued · waiting for workers");

    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(ran);
}

TEST_CASE("TaskManager: cancelling a queued task removes it", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   release(1);
    bool         removedSeen = false;
    bool         ran         = false;
    tasks.submit(writeTask("first", {1}, [&](TaskContext&) { release.wait(); }));
    const TaskId queued = tasks.submit(writeTask("second", {1}, [&](TaskContext&) { ran = true; }));
    tasks.setListener([&](const TaskSnapshot& snapshot, bool removed) {
        if (snapshot.id == queued && removed)
            removedSeen = true;
    });

    tasks.cancel(queued);
    CHECK_FALSE(tasks.snapshot(queued));
    CHECK(removedSeen);
    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK_FALSE(ran);
}

TEST_CASE("TaskManager: cancelling a running task skips its apply", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    bool         applied = false;
    const TaskId id      = tasks.submit(writeTask(
        "long", {1},
        [](TaskContext& context) {
            while (!context.cancelled())
                std::this_thread::sleep_for(1ms);
        },
        [&] { applied = true; }));

    tasks.cancel(id);
    CHECK(stateOf(tasks, id) == TaskState::Cancelling);
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK_FALSE(applied);
    CHECK_FALSE(tasks.snapshot(id));
}

TEST_CASE("TaskManager: cancelling after work finished but before apply skips apply", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    bool         applied = false;
    const TaskId id      = tasks.submit(writeTask("quick", {1}, [](TaskContext&) { }, [&] { applied = true; }));

    REQUIRE(waitFor([&] { return ui.size() > 0; })); // finish posted, not yet run
    tasks.cancel(id);
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK_FALSE(applied);
}

TEST_CASE("TaskManager: a failure becomes a Failed card and unblocks the lane", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    bool         nextApplied = false;
    const TaskId failing     = tasks.submit(writeTask("cut", {1}, [](TaskContext&) { throw std::runtime_error("boom"); }));
    tasks.submit(writeTask("move", {1}, [](TaskContext&) { }, [&] { nextApplied = true; }));

    REQUIRE(ui.drainUntil([&] { return nextApplied; }));
    REQUIRE(stateOf(tasks, failing) == TaskState::Failed);
    CHECK(tasks.snapshot(failing)->error == "boom");
    CHECK(tasks.idle()); // a Failed card is not pending work
    tasks.dismiss(failing);
    CHECK_FALSE(tasks.snapshot(failing));
}

TEST_CASE("TaskManager: a prepare error fails the task without running its work", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    bool         ran  = false;
    Task         task = writeTask("move", {1}, [&](TaskContext&) { ran = true; });
    task.prepare      = [] { return std::optional<std::string>("model \"cube\" no longer exists"); };
    const TaskId id   = tasks.submit(std::move(task));

    REQUIRE(stateOf(tasks, id) == TaskState::Failed);
    CHECK(tasks.snapshot(id)->error == "model \"cube\" no longer exists");
    CHECK_FALSE(ran);
}

TEST_CASE("TaskManager: synchronous mode runs every step inline", "[application][tasks]")
{
    TaskManager tasks;
    bool        applied = false;
    tasks.submit(writeTask("import", {1}, [](TaskContext&) { }, [&] { applied = true; }));
    CHECK(applied);

    const TaskId failing = tasks.submit(writeTask("export", {2}, [](TaskContext&) { throw std::runtime_error("disk full"); }));
    REQUIRE(stateOf(tasks, failing) == TaskState::Failed);
    CHECK(tasks.snapshot(failing)->error == "disk full");
}

TEST_CASE("TaskManager: cancelAllAndWait waits for running work", "[application][tasks]")
{
    ManualPoster      ui;
    bool              applied = false;
    std::atomic<bool> stopped = false;
    {
        TaskManager tasks(ui.poster());
        tasks.submit(writeTask(
            "long", {1},
            [&](TaskContext& context) {
                while (!context.cancelled())
                    std::this_thread::sleep_for(1ms);
                stopped = true;
            },
            [&] { applied = true; }));

        tasks.cancelAllAndWait();
        CHECK(stopped);
        CHECK(tasks.idle());
    }
    ui.drainUntil([] { return false; }, 50ms); // late callbacks find the manager gone and do nothing
    CHECK_FALSE(applied);
}

TEST_CASE("TaskManager: progress notifications are throttled", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    int          progressNotifications = 0;
    tasks.setListener([&](const TaskSnapshot& snapshot, bool removed) {
        if (!removed && snapshot.state == TaskState::Running && snapshot.progress)
            ++progressNotifications;
    });

    const auto start = std::chrono::steady_clock::now();
    // Reports as fast as it can for 300 ms: a fixed duration, not a number of
    // sleeps (Windows rounds short sleeps up to 1-15 ms).
    tasks.submit(writeTask("busy", {1}, [](TaskContext& context) {
        const auto begin = std::chrono::steady_clock::now();
        for (auto now = begin; now - begin < 300ms; now = std::chrono::steady_clock::now())
        {
            context.setProgress(std::chrono::duration<double>(now - begin) / 300ms);
            std::this_thread::yield();
        }
    }));
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }, 30s));
    const auto elapsed = std::chrono::steady_clock::now() - start;

    CHECK(progressNotifications >= 1);
    CHECK(progressNotifications <= elapsed / 50ms + 2);
}

TEST_CASE("TaskManager: a queued read blocks a later write until it has its snapshot", "[application][tasks]")
{
    ManualPoster     ui;
    TaskManager      tasks(ui.poster());
    std::latch       release(1);
    std::vector<int> order;

    tasks.submit(writeTask("import", {1}, [&](TaskContext&) { release.wait(); }, [&] { order.push_back(1); }));
    tasks.submit(readTask("export", {1}, [](TaskContext&) { }, [&] { order.push_back(2); })); // queued, no snapshot yet
    CHECK(tasks.wouldWait(1));
    const TaskId move = tasks.submit(writeTask("move", {1}, {}, [&] { order.push_back(3); }));

    REQUIRE(stateOf(tasks, move) == TaskState::Queued);                       // not applied under the export's feet
    CHECK(tasks.snapshot(move)->status == "Queued · waiting for \"import\""); // the earliest blocker
    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(order == std::vector<int>{1, 2, 3});
}

TEST_CASE("TaskManager: a throwing prepare or apply fails its task and keeps scheduling", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   release(1);
    bool         lastApplied = false;

    tasks.submit(writeTask("first", {1}, [&](TaskContext&) { release.wait(); }));
    Task         badApply   = writeTask("bad apply", {1}, {}, [] { throw std::runtime_error("apply broke"); });
    const TaskId bad1       = tasks.submit(std::move(badApply));
    Task         badPrepare = writeTask("bad prepare", {1});
    badPrepare.prepare      = []() -> std::optional<std::string> { throw std::runtime_error("prepare broke"); };
    const TaskId bad2       = tasks.submit(std::move(badPrepare));
    tasks.submit(writeTask("last", {1}, [](TaskContext&) { }, [&] { lastApplied = true; }));

    release.count_down();
    REQUIRE(ui.drainUntil([&] { return lastApplied; }));
    CHECK(tasks.snapshot(bad1)->error == "apply broke");
    CHECK(tasks.snapshot(bad2)->error == "prepare broke");
}

TEST_CASE("TaskManager: a throwing instant edit fails without blocking later ones", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    const TaskId bad = tasks.submit(writeTask("move", {1}, {}, [] { throw std::runtime_error("broke"); }));
    REQUIRE(stateOf(tasks, bad) == TaskState::Failed);
    CHECK(tasks.snapshot(bad)->error == "broke");

    bool applied = false;
    tasks.submit(writeTask("move again", {1}, {}, [&] { applied = true; }));
    CHECK(applied);
}

TEST_CASE("TaskManager: a throwing listener breaks nothing", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    tasks.setListener([](const TaskSnapshot&, bool) { throw std::runtime_error("ui broke"); });
    bool applied = false;
    REQUIRE_NOTHROW(tasks.submit(writeTask("t", {1}, [](TaskContext&) { }, [&] { applied = true; })));
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(applied);
}

TEST_CASE("TaskManager: tasks submitted together wait while the first takes its workers", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   inside(1);
    std::latch   release(1);
    auto         make = [](std::string name, application::service::ModelId id, std::function<void(TaskContext&)> work) {
        Task task      = writeTask(std::move(name), {id}, std::move(work));
        task.operation = "test.together"; // unknown: the first takes every worker
        return task;
    };
    tasks.submit(make("a", 1, [&](TaskContext&) {
        inside.count_down();
        release.wait();
    }));
    const TaskId b = tasks.submit(make("b", 2, [](TaskContext&) { }));
    const TaskId c = tasks.submit(make("c", 3, [](TaskContext&) { }));

    CHECK(stateOf(tasks, b) == TaskState::Queued);
    CHECK(stateOf(tasks, c) == TaskState::Queued);
    inside.wait();
    REQUIRE(ui.drainUntil([&] { return tasks.snapshot(b)->status == "Queued · waiting for workers"; }));
    CHECK(stateOf(tasks, c) == TaskState::Queued);

    release.count_down();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
}

TEST_CASE("TaskManager: submit after cancelAllAndWait is refused", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    tasks.cancelAllAndWait();
    bool applied = false;
    CHECK(tasks.submit(writeTask("late", {1}, {}, [&] { applied = true; })) == 0);
    CHECK_FALSE(applied);
}

TEST_CASE("TaskManager: discard runs whenever a task ends without applying", "[application][tasks]")
{
    serial("test.work"); // one worker each, so the tasks below run side by side
    ManualPoster ui;
    std::latch   release(1);
    int          appliedDiscards = 0;
    int          queuedDiscards  = 0;
    int          failedDiscards  = 0;
    int          runningDiscards = 0;
    int          shutdownDiscard = 0;
    {
        TaskManager tasks(ui.poster());
        auto        with = [](Task task, int& counter) {
            task.discard = [&counter] { ++counter; };
            return task;
        };

        tasks.submit(with(writeTask("applied", {1}, [](TaskContext&) { }), appliedDiscards));
        tasks.submit(with(writeTask("blocker", {2}, [&](TaskContext&) { release.wait(); }), runningDiscards));
        const TaskId queued = tasks.submit(with(writeTask("queued", {2}, [](TaskContext&) { }), queuedDiscards));
        tasks.submit(with(writeTask("failing", {3}, [](TaskContext&) { throw std::runtime_error("boom"); }), failedDiscards));
        tasks.cancel(queued);
        CHECK(queuedDiscards == 1);

        REQUIRE(ui.drainUntil([&] { return failedDiscards == 1 && appliedDiscards == 0 && tasks.pending() == 1; }));
        CHECK(appliedDiscards == 0);

        Task looping = with(writeTask("looping", {4},
                                      [](TaskContext& context) {
                                          while (!context.cancelled())
                                              std::this_thread::sleep_for(1ms);
                                      }),
                            shutdownDiscard);
        tasks.submit(std::move(looping));
        release.count_down();
        REQUIRE(ui.drainUntil([&] { return tasks.pending() == 1; })); // blocker applied, looping runs
        tasks.cancelAllAndWait();
    }
    CHECK(runningDiscards == 0); // the blocker was applied
    CHECK(shutdownDiscard == 1);
    CHECK(queuedDiscards == 1);
    CHECK(failedDiscards == 1);
}

TEST_CASE("TaskManager: synchronous mode discards a failed task", "[application][tasks]")
{
    TaskManager tasks;
    int         discards = 0;
    Task        task     = writeTask("export", {1}, [](TaskContext&) { throw std::runtime_error("disk full"); });
    task.discard         = [&] { ++discards; };
    tasks.submit(std::move(task));
    CHECK(discards == 1);
}

TEST_CASE("TaskManager: workers freed by work outside it start its waiting tasks", "[application][tasks]")
{
    ManualPoster ui;
    TaskManager  tasks(ui.poster());
    std::latch   holding(1);
    std::latch   release(1);

    // Hold every worker with a plain run(), as code outside any TaskManager could.
    std::thread outside([&] {
        Dispatcher::getInstance().run("test.outside", [&] {
            holding.count_down();
            release.wait();
        });
    });
    holding.wait();
    REQUIRE(Dispatcher::getInstance().freeWorkers() == 0);

    bool         ran     = false;
    const TaskId waiting = tasks.submit(writeTask("waiting", {1}, [&](TaskContext&) { ran = true; }));
    REQUIRE(tasks.snapshot(waiting)->status == "Queued · waiting for workers");

    release.count_down(); // no TaskManager event follows: only the freed workers can wake it
    outside.join();
    REQUIRE(ui.drainUntil([&] { return tasks.idle(); }));
    CHECK(ran);
}

TEST_CASE("TaskManager: destroyed while outside runs keep freeing workers", "[application][tasks]")
{
    ManualPoster      ui;
    std::atomic<bool> stop = false;
    std::thread       outside([&] {
        while (!stop)
            Dispatcher::getInstance().run("test.churn", [] { });
    });
    for (int i = 0; i < 50; ++i)
    {
        TaskManager tasks(ui.poster());
        tasks.submit(writeTask("t", {1}, [](TaskContext&) { }));
    } // each destroyed while releases keep coming
    stop = true;
    outside.join();
    ui.drainUntil([] { return false; }, std::chrono::milliseconds(50)); // stale reschedules: must do nothing
    SUCCEED("no callback reached a destroyed TaskManager");
}
