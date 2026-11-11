#include "application/tasks/model_tasks.hpp"
#include "application/tasks/scene_editor.hpp"
#include "application/tasks/task_manager.hpp"
#include "platform/threading/dispatcher.hpp"
#include "support/manual_poster.hpp"
#include "support/temp_dir.hpp"

#include <TopExp_Explorer.hxx>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <string>
#include <thread>

using application::service::SceneService;
using application::tasks::SceneServiceEditor;
using application::tasks::TaskManager;
using application::tasks::TaskState;
using geometry::occt::Shape;
using geometry::occt::Transform;
namespace tasks = application::tasks;

namespace
{
    const std::filesystem::path kCube = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "ascii" / "cube.stl";

    int countFaces(const Shape& shape)
    {
        int count = 0;
        for (TopExp_Explorer it(shape.value(), TopAbs_FACE); it.More(); it.Next())
            ++count;
        return count;
    }

    // Runs tasks inline against a plain SceneService.
    struct SyncFixture
    {
        SceneService       scene;
        SceneServiceEditor editor{scene};
        TaskManager        manager;

        std::string failure(tasks::TaskId id) const
        {
            const auto snapshot = manager.snapshot(id);
            return snapshot && snapshot->state == TaskState::Failed ? snapshot->error : std::string();
        }
    };
} // namespace

TEST_CASE_METHOD(SyncFixture, "Model tasks: import adds the model under its reserved id", "[application][tasks]")
{
    const auto task = tasks::importTask(scene, editor, kCube);
    REQUIRE(task.writes.size() == 1);
    const auto reserved = task.writes.front();
    CHECK(task.name == "Import cube.stl");

    manager.submit(task);
    REQUIRE(scene.find(reserved) != nullptr);
    CHECK(scene.find(reserved)->name == "cube.stl");
    CHECK(scene.find(reserved)->loadDuration.count() > 0.0);
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: importing a missing file fails", "[application][tasks]")
{
    const auto id = manager.submit(tasks::importTask(scene, editor, kCube.parent_path() / "missing.stl"));
    CHECK_FALSE(failure(id).empty());
    CHECK(scene.empty());
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: export writes the file", "[application][tasks]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    manager.submit(tasks::importTask(scene, editor, kCube));

    const auto task = tasks::exportTask(scene, output);
    CHECK(task.name == "Export out.stl");
    const auto id = manager.submit(task);
    CHECK(failure(id).empty());
    CHECK(std::filesystem::exists(output));
    CHECK_FALSE(tests::support::hasTemporaryFiles(dir.path()));
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: exporting an empty scene fails", "[application][tasks]")
{
    const tests::support::TempDir dir;
    const auto                    id = manager.submit(tasks::exportTask(scene, dir.path() / "out.stl"));
    CHECK(failure(id) == "no model loaded");
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: move then remove", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube));
    const auto id = scene.models()[0].id;

    const auto move = tasks::moveTask(scene, editor, id, Transform{.moveX = 5});
    CHECK(move.name == "Move cube.stl");
    manager.submit(move);
    CHECK(scene.transform(0).moveX == 5);

    const auto remove = tasks::removeTask(scene, editor, id);
    CHECK(remove.name == "Remove cube.stl");
    manager.submit(remove);
    CHECK(scene.empty());
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: a move after a remove fails", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube));
    const auto id = scene.models()[0].id;

    auto move = tasks::moveTask(scene, editor, id, Transform{.moveX = 5}); // created while the model exists
    manager.submit(tasks::removeTask(scene, editor, id));
    const auto moved = manager.submit(std::move(move));
    CHECK(failure(moved) == "model \"cube.stl\" no longer exists");
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: clear removes every model", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube));
    manager.submit(tasks::importTask(scene, editor, kCube));
    const auto task = tasks::clearTask(scene, editor);
    CHECK(task.name == "Clear scene");
    manager.submit(task);
    CHECK(scene.empty());
}

TEST_CASE("Model tasks: an export waits for an import asked before it, and includes it", "[application][tasks]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    tests::support::ManualPoster  ui;
    SceneService                  scene;
    SceneServiceEditor            editor(scene);
    TaskManager                   manager(ui.poster());

    manager.submit(tasks::importTask(scene, editor, kCube));
    manager.submit(tasks::exportTask(scene, output));
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));

    REQUIRE(scene.size() == 1);
    CHECK(countFaces(Shape::fromSTL(output)) == 12);
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: a failed import releases its reserved id", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube.parent_path() / "missing.stl"));
    CHECK(scene.reservedIds().empty());
}

TEST_CASE("Model tasks: cancelling an export after its work leaves no file", "[application][tasks]")
{
    const tests::support::TempDir dir;
    const auto                    output = dir.path() / "out.stl";
    tests::support::ManualPoster  ui;
    SceneService                  scene;
    SceneServiceEditor            editor(scene);
    TaskManager                   manager(ui.poster());
    scene.add("cube", Shape::fromSTL(kCube));

    const auto id   = manager.submit(tasks::exportTask(scene, output));
    auto&      pool = platform::threading::Dispatcher::getInstance();
    // The work is over once something was written and its workers are free.
    const auto done = [&] {
        return (std::filesystem::exists(output) || tests::support::hasTemporaryFiles(dir.path())) &&
               pool.freeWorkers() == pool.total();
    };
    for (int i = 0; i < 5000 && !done(); ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    REQUIRE(done());
    std::this_thread::sleep_for(std::chrono::milliseconds(20)); // its result gets posted
    manager.cancel(id);
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));

    CHECK_FALSE(std::filesystem::exists(output));
    CHECK_FALSE(tests::support::hasTemporaryFiles(dir.path()));
}
