#include "application/tasks/model_tasks.hpp"
#include "application/tasks/scene_editor.hpp"
#include "application/tasks/task_manager.hpp"
#include "platform/threading/dispatcher.hpp"
#include "support/manual_poster.hpp"
#include "support/temp_dir.hpp"

#include <TopExp_Explorer.hxx>
#include <TopoDS_TShape.hxx>
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
        return shape.triangleCount(); // imported models are meshes: count their triangles
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

namespace
{
    // A stand-in for a B-Rep operation (cut, analysis): reads the model's
    // B-Rep in prepare, through requireBRep().
    tasks::Task brepOperation(const SceneService& scene, application::service::ModelId id, int& sawBRep)
    {
        tasks::Task task;
        task.name    = "Analyze";
        task.reads   = {id};
        auto require = tasks::requireBRep(scene, id);
        task.prepare = [require, &scene, id, &sawBRep]() -> std::optional<std::string> {
            if (auto error = require())
                return error;
            if (scene.brep(id) != nullptr)
                ++sawBRep;
            return std::nullopt;
        };
        task.apply = [] { };
        return task;
    }
} // namespace

TEST_CASE_METHOD(SyncFixture, "Model tasks: convert caches a B-Rep", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube));
    const auto id   = scene.models()[0].id;
    const auto task = tasks::convertTask(scene, editor, id);
    CHECK(task.name == "Convert cube.stl");
    CHECK(task.operation == "model.convert");

    const auto converted = manager.submit(task);
    CHECK(failure(converted).empty());
    REQUIRE(scene.brep(id) != nullptr);
    CHECK_FALSE(scene.models()[0].shape.mesh().IsNull());
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: converting a removed model fails", "[application][tasks]")
{
    manager.submit(tasks::importTask(scene, editor, kCube));
    const auto id      = scene.models()[0].id;
    auto       convert = tasks::convertTask(scene, editor, id);
    manager.submit(tasks::removeTask(scene, editor, id));
    CHECK(failure(manager.submit(std::move(convert))) == "model \"cube.stl\" no longer exists");
}

TEST_CASE("Model tasks: a cancelled conversion caches nothing", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;

    manager.cancel(manager.submit(tasks::convertTask(scene, editor, id)));
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
    CHECK(scene.brep(id) == nullptr);
}

TEST_CASE("Model tasks: submitWithBRep converts once for two operations", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id      = scene.models()[0].id;
    int        sawBRep = 0;

    const auto first  = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    const auto second = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    CHECK(first.size() == 2); // the conversion, then the operation
    CHECK(manager.snapshot(first[0])->name == "Convert cube.stl");
    CHECK(second.size() == 1); // the pending conversion is shared
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
    CHECK(sawBRep == 2);

    const auto third = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    CHECK(third.size() == 1); // already converted
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
    CHECK(sawBRep == 3);
}

TEST_CASE("Model tasks: an operation after a cancelled conversion fails", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id      = scene.models()[0].id;
    int        sawBRep = 0;

    const auto ids = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    REQUIRE(ids.size() == 2);
    manager.cancel(ids[0]);
    REQUIRE(ui.drainUntil([&] { return manager.pending() == 0; }));
    const auto failed = manager.snapshot(ids[1]);
    REQUIRE(failed);
    CHECK(failed->error == "model \"cube.stl\" could not be converted");
    CHECK(sawBRep == 0);
}

TEST_CASE("Model tasks: large imports learn their own scalability", "[application][tasks]")
{
    // Small files barely use a second core: if they shared a profile with big
    // ones, the Dispatcher would learn that importing never scales.
    CHECK(tasks::importOperation(0) == "model.import");
    CHECK(tasks::importOperation(tasks::kLargeImportBytes - 1) == "model.import");
    CHECK(tasks::importOperation(tasks::kLargeImportBytes) == "model.import.large");

    SceneService       scene;
    SceneServiceEditor editor(scene);
    CHECK(tasks::importTask(scene, editor, kCube).operation == "model.import");
}

TEST_CASE("Model tasks: a running conversion doesn't hold up edits of its model", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;

    manager.submit(tasks::convertTask(scene, editor, id)); // running: its result waits in the poster
    manager.submit(tasks::moveTask(scene, editor, id, Transform{.moveX = 5}));
    CHECK(scene.transform(0).moveX == 5); // applied at once: a conversion only reads the model

    int        sawBRep = 0;
    const auto ids     = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    REQUIRE(ids.size() == 1);                                                        // the running conversion is reused...
    CHECK(manager.snapshot(ids[0])->state == application::tasks::TaskState::Queued); // ...and waited for
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
    CHECK(scene.brep(id) != nullptr);
    CHECK(sawBRep == 1);
}

TEST_CASE("Model tasks: removing a model mid-conversion drops the result", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;

    manager.submit(tasks::convertTask(scene, editor, id));
    manager.submit(tasks::removeTask(scene, editor, id));
    CHECK(scene.empty()); // not held up by the conversion
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
    CHECK(scene.brep(id) == nullptr);
}

TEST_CASE("Model tasks: retrying after cancelling a conversion converts again", "[application][tasks]")
{
    tests::support::ManualPoster ui;
    SceneService                 scene;
    SceneServiceEditor           editor(scene);
    TaskManager                  manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id      = scene.models()[0].id;
    int        sawBRep = 0;

    const auto first = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    REQUIRE(first.size() == 2);
    manager.cancel(first[0]); // Cancelling until its work notices
    const auto retry = tasks::submitWithBRep(manager, scene, editor, {id}, brepOperation(scene, id, sawBRep));
    CHECK(retry.size() == 2); // a fresh conversion, not the one being cancelled
    REQUIRE(ui.drainUntil([&] { return manager.pending() == 0; }));
    CHECK(sawBRep == 1); // the retried operation got its B-Rep
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: converting a model that is already a B-Rep caches it", "[application][tasks]")
{
    scene.add("cut.brep", Shape::toBRep(Shape::fromSTL(kCube))); // e.g. a future cut's result
    const auto id = scene.models()[0].id;
    CHECK(failure(manager.submit(tasks::convertTask(scene, editor, id))).empty());
    REQUIRE(scene.brep(id) != nullptr);
    CHECK(scene.brep(id)->value().IsSame(scene.models()[0].shape.value()));
}

TEST_CASE_METHOD(SyncFixture, "Model tasks: a model still importing is named after its file", "[application][tasks]")
{
    const auto reserved = scene.reserveId("lucy.stl");
    CHECK(tasks::convertTask(scene, editor, reserved).name == "Convert lucy.stl");
}

TEST_CASE("Model tasks: an export never holds the model's B-Rep", "[application][tasks]")
{
    const tests::support::TempDir dir;
    tests::support::ManualPoster  ui;
    SceneService                  scene;
    SceneServiceEditor            editor(scene);
    TaskManager                   manager(ui.poster());
    scene.add("cube.stl", Shape::fromSTL(kCube));
    const auto id = scene.models()[0].id;
    scene.setBRep(id, Shape::toBRep(scene.models()[0].shape));
    const occ::handle<TopoDS_TShape> brep  = scene.brep(id)->value().TShape();
    const int                        owned = brep->GetRefCount();

    manager.submit(tasks::exportTask(scene, dir.path() / "out.stl"));
    CHECK(brep->GetRefCount() == owned); // the export's snapshot left it out
    REQUIRE(ui.drainUntil([&] { return manager.idle(); }));
}
