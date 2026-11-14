#include "application/tasks/task.hpp"
#include "geometry/occt/shape.hpp"
#include "geometry/occt/transform.hpp"
#include "platform/logger/logger.hpp"
#include "platform/settings/settings_registry.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <gp_Pnt.hxx>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <wx/glcanvas.h>
#include <wx/sizer.h>

// Exercises the real OCCT viewer (OpenGL context, AIS presentations, manipulator)
// inside a shown frame. On headless Linux this needs Xvfb + Mesa (see Tests.py).

using Catch::Matchers::ContainsSubstring;
using tests::support::ScopedFrame;
using ui::framework::occt::ModelViewerPanel;

namespace
{
    const std::filesystem::path kCube = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "ascii" / "cube.stl";

    class ViewerFixture
    {
      public:
        ViewerFixture()
            : frame(nullptr, wxID_ANY, "ModelViewerPanel test", wxDefaultPosition, wxSize(640, 480))
            , panel(new ModelViewerPanel(frame.get()))
        {
            auto* sizer = new wxBoxSizer(wxVERTICAL);
            sizer->Add(panel, 1, wxEXPAND);
            frame->SetSizer(sizer);
            frame.showAndWait();

            panel->SetOnModelStateChanged([this](bool hasModel) { states.push_back(hasModel); });
            // A real dialog would block the test until someone clicks it.
            panel->SetConfirmConvertLoaded([](ModelViewerPanel::LoadedModelsQuestion, std::size_t) {
                FAIL_CHECK("unexpected \"convert loaded models?\" dialog");
                return false;
            });
        }

        // A task that holds `id` until cancelled: anything later on that model queues behind it.
        application::tasks::TaskId hold(application::service::ModelId id)
        {
            application::tasks::Task task;
            task.name      = "hold";
            task.operation = "test.hold";
            task.writes    = {id};
            task.work      = [](application::tasks::TaskContext& context) {
                while (!context.cancelled())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            };
            task.apply = [] { };
            return panel->tasks().submit(std::move(task));
        }

        // Waits until every task the panel started is over (tasks apply on the
        // UI thread, through events this pumps).
        bool settle()
        {
            return tests::support::pumpUntil([this] { return panel->tasks().idle(); });
        }

        ScopedFrame<>     frame;
        ModelViewerPanel* panel;
        std::vector<bool> states;
    };
} // namespace

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: loading models", "[integration][ui][viewer]")
{
    REQUIRE_FALSE(panel->hasModel());

    REQUIRE(panel->loadFile(kCube));
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());

    CHECK(panel->hasModel());
    CHECK(panel->GetModelNames() == std::vector<wxString>{"cube.stl", "cube.stl (2)"});
    CHECK(states == std::vector<bool>{true, true});
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: a failed load changes nothing", "[integration][ui][viewer]")
{
    REQUIRE(panel->loadFile(kCube.parent_path() / "missing.stl"));
    REQUIRE(settle());

    CHECK_FALSE(panel->hasModel());
    CHECK(states.empty());
    const auto cards = panel->tasks().snapshots();
    REQUIRE(cards.size() == 1);
    CHECK(cards[0].state == application::tasks::TaskState::Failed);
    CHECK_FALSE(cards[0].error.empty());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: transforms are applied per target", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    REQUIRE(settle());
    panel->loadFile(kCube);
    REQUIRE(settle());

    panel->setTransform(1, 10, 20, 30, 0, 0, 90);
    CHECK(panel->GetTransform(0) == ModelViewerPanel::TransformValues{});
    CHECK(panel->GetTransform(1) == ModelViewerPanel::TransformValues{10, 20, 30, 0, 0, 90});

    panel->setTransform(-1, 1, 2, 3, 4, 5, 6);
    CHECK(panel->GetTransform(0) == ModelViewerPanel::TransformValues{1, 2, 3, 4, 5, 6});
    CHECK(panel->GetTransform(1) == ModelViewerPanel::TransformValues{1, 2, 3, 4, 5, 6});
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: gizmo can be shown, switched and hidden", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    REQUIRE(settle());

    // Builds/attaches a fresh AIS_Manipulator each time (see setGizmoTarget());
    // a stale-selection bug here used to segfault inside OCCT.
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Move);
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Rotate);
    panel->setTransform(0, 5, 0, 0, 0, 0, 45); // repositions the visible gizmo
    panel->SetGizmoState(-1, ModelViewerPanel::GizmoTool::Move);
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::None);
    tests::support::pumpEvents();

    CHECK(panel->GetTransform(0) == ModelViewerPanel::TransformValues{5, 0, 0, 0, 0, 45});
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: export and new file", "[integration][ui][viewer]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "export.stl";

    CHECK_FALSE(panel->exportFile(output)); // nothing loaded yet

    panel->loadFile(kCube);
    REQUIRE(settle());
    CHECK(panel->exportFile(output));
    REQUIRE(settle());
    CHECK(std::filesystem::file_size(output) > 0);

    panel->newFile();
    REQUIRE(settle());
    CHECK_FALSE(panel->hasModel());
    CHECK(panel->GetModelNames().empty());
    CHECK(states == std::vector<bool>{true, false});
}

namespace
{
    // Sends a mouse event to the viewer's GL canvas, as the user's mouse would.
    void sendMouse(wxWindow* canvas, wxEventType type, int x, int y, int wheelRotation = 0)
    {
        wxMouseEvent event(type);
        event.SetEventObject(canvas);
        event.SetPosition(wxPoint(x, y));
        event.m_x = x;
        event.m_y = y;
        if (type == wxEVT_MOTION)
            event.m_leftDown = true;
        event.m_wheelRotation = wheelRotation;
        event.m_wheelDelta    = 120;
        canvas->HandleWindowEvent(event);
    }
} // namespace

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: camera controls never move the model", "[integration][ui][viewer]")
{
    auto* canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    REQUIRE(canvas != nullptr);
    panel->loadFile(kCube);
    REQUIRE(settle());
    panel->setTransform(0, 1, 2, 3, 0, 0, 0);

    SECTION("left drag orbits")
    {
        sendMouse(canvas, wxEVT_LEFT_DOWN, 100, 100);
        sendMouse(canvas, wxEVT_MOTION, 140, 120);
        sendMouse(canvas, wxEVT_MOTION, 180, 140);
        sendMouse(canvas, wxEVT_LEFT_UP, 180, 140);
    }

    SECTION("middle drag pans")
    {
        sendMouse(canvas, wxEVT_MIDDLE_DOWN, 100, 100);
        sendMouse(canvas, wxEVT_MOTION, 130, 90);
        sendMouse(canvas, wxEVT_MIDDLE_UP, 130, 90);
    }

    SECTION("wheel zooms in and out")
    {
        for (int i = 0; i < 30; ++i) // past the zoom clamp
            sendMouse(canvas, wxEVT_MOUSEWHEEL, 100, 100, 120);
        sendMouse(canvas, wxEVT_MOUSEWHEEL, 100, 100, -120);
        // Trackpad-sized deltas (well below one notch) and empty events.
        sendMouse(canvas, wxEVT_MOUSEWHEEL, 100, 100, 3);
        sendMouse(canvas, wxEVT_MOUSEWHEEL, 100, 100, -3);
        sendMouse(canvas, wxEVT_MOUSEWHEEL, 100, 100, 0);
    }

    SECTION("moving without a button pressed does nothing")
    {
        wxMouseEvent move(wxEVT_MOTION);
        move.SetEventObject(canvas);
        canvas->HandleWindowEvent(move);
    }

    SECTION("resizing")
    {
        frame->SetSize(800, 600);
        tests::support::pumpEvents();
    }

    SECTION("dragging with the gizmo active but missing it falls back to orbiting")
    {
        panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Move);
        sendMouse(canvas, wxEVT_LEFT_DOWN, 5, 5); // a corner, far from the gizmo
        sendMouse(canvas, wxEVT_MOTION, 40, 30);
        sendMouse(canvas, wxEVT_LEFT_UP, 40, 30);
    }

    tests::support::pumpEvents();
    CHECK_FALSE(canvas->HasCapture());
    CHECK(panel->GetTransform(0) == ModelViewerPanel::TransformValues{1, 2, 3, 0, 0, 0});
}

TEST_CASE("ModelViewerPanel: mouse input before any model is loaded is harmless", "[integration][ui][viewer]")
{
    ScopedFrame<> frame(nullptr, wxID_ANY, "Uninitialized viewer", wxDefaultPosition, wxSize(320, 240));
    auto*         panel  = new ModelViewerPanel(frame.get());
    auto*         canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    REQUIRE(canvas != nullptr);

    // Not shown, so the OCCT viewer was never created: handlers must bail out.
    sendMouse(canvas, wxEVT_MOTION, 10, 10);
    sendMouse(canvas, wxEVT_MOUSEWHEEL, 10, 10, 120);
    panel->setTransform(0, 1, 0, 0, 0, 0, 0);
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Move);

    CHECK_FALSE(panel->hasModel());
    CHECK(panel->GetModelNames().empty());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: logs import and display times", "[integration][ui][viewer]")
{
    std::ostringstream captured;
    std::streambuf*    previous = std::clog.rdbuf(captured.rdbuf());
    platform::logger::logger::setDebug(true); // what Bootstrap does in the app

    const bool loaded = panel->loadFile(kCube);
    REQUIRE(settle());

    platform::logger::logger::setDebug(false);
    std::clog.rdbuf(previous);

    REQUIRE(loaded);
    const std::string log = captured.str();
    CHECK_THAT(log, ContainsSubstring("[scene] imported 'cube.stl' from ") && ContainsSubstring(" ms"));
    CHECK_THAT(log, ContainsSubstring("[viewer] displayed 'cube.stl' (time ") && ContainsSubstring(", total "));
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: show and hide models", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    panel->loadFile(kCube);
    REQUIRE(settle());
    REQUIRE(panel->GetModelVisibility() == std::vector<bool>{true, true});

    panel->SetModelVisibility(0, false);
    CHECK(panel->GetModelVisibility() == std::vector<bool>{false, true});

    panel->SetModelVisibility(0, true);
    CHECK(panel->GetModelVisibility() == std::vector<bool>{true, true});

    // Out of range: ignored.
    panel->SetModelVisibility(5, false);
    panel->SetModelVisibility(-1, false);
    CHECK(panel->GetModelVisibility() == std::vector<bool>{true, true});

    // A hidden model stays in the scene and keeps its transform.
    panel->SetModelVisibility(1, false);
    panel->setTransform(1, 3, 0, 0, 0, 0, 0);
    CHECK(panel->GetModelNames().size() == 2);
    CHECK(panel->GetTransform(1).moveX == 3.0);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: removing a model", "[integration][ui][viewer]")
{
    tests::support::TempDir tmp;
    panel->loadFile(kCube);
    panel->loadFile(kCube);
    panel->loadFile(kCube);
    REQUIRE(settle());
    panel->setTransform(2, 7, 0, 0, 0, 0, 0);
    panel->SetModelVisibility(2, false);
    states.clear();

    panel->RemoveModel(1);

    CHECK(panel->GetModelNames() == std::vector<wxString>{"cube.stl", "cube.stl (3)"});
    // The models after it shift down with their transform and visibility.
    CHECK(panel->GetTransform(1).moveX == 7.0);
    CHECK(panel->GetModelVisibility() == std::vector<bool>{true, false});
    CHECK(states == std::vector<bool>{true});

    // The scene and the viewport stay in step: export sees 2 cubes.
    REQUIRE(panel->exportFile(tmp.path() / "out.stl"));
    REQUIRE(settle());
    CHECK(geometry::occt::Shape::fromSTL(tmp.path() / "out.stl").isNull() == false);

    panel->RemoveModel(9); // nothing removed (the state is still re-notified)
    CHECK(panel->GetModelNames().size() == 2);
    panel->RemoveModel(0);
    panel->RemoveModel(0);
    CHECK_FALSE(panel->hasModel());
    CHECK(states == std::vector<bool>{true, true, true, false});
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: removing the gizmo's model hides the gizmo", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    panel->loadFile(kCube);
    REQUIRE(settle());
    panel->SetGizmoState(1, ModelViewerPanel::GizmoTool::Move);

    panel->RemoveModel(1);
    // The gizmo must not point at a removed (or shifted) model any more.
    panel->setTransform(0, 1, 0, 0, 0, 0, 0);
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Rotate);

    CHECK(panel->GetTransform(0).moveX == 1.0);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: focusing a model keeps visibility", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    panel->loadFile(kCube);
    REQUIRE(settle());
    panel->setTransform(1, 100, 0, 0, 0, 0, 0);
    panel->SetModelVisibility(0, false);

    panel->FocusModel(1);
    panel->FocusModel(0); // a hidden model can be framed too
    panel->FocusModel(4); // ignored

    CHECK(panel->GetModelVisibility() == std::vector<bool>{false, true});
}

TEST_CASE("ModelViewerPanel: model management before the viewer exists is harmless", "[integration][ui][viewer]")
{
    ScopedFrame<> frame(nullptr, wxID_ANY, "Uninitialized viewer", wxDefaultPosition, wxSize(320, 240));
    auto*         panel = new ModelViewerPanel(frame.get());

    panel->SetModelVisibility(0, false);
    panel->RemoveModel(0);
    panel->FocusModel(0);

    CHECK(panel->GetModelVisibility().empty());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: loadFile returns before the model is loaded", "[integration][ui][viewer][tasks]")
{
    REQUIRE(panel->loadFile(kCube));
    CHECK_FALSE(panel->hasModel()); // still importing in the background
    REQUIRE(settle());
    CHECK(panel->hasModel());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: cancelling an import adds nothing", "[integration][ui][viewer][tasks]")
{
    REQUIRE(panel->loadFile(kCube));
    const auto cards = panel->tasks().snapshots();
    REQUIRE(cards.size() == 1);
    panel->tasks().cancel(cards[0].id);
    REQUIRE(settle());
    CHECK_FALSE(panel->hasModel());
    CHECK(states.empty());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: a cancelled export leaves no file", "[integration][ui][viewer][tasks]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "out.stl";
    panel->loadFile(kCube);
    REQUIRE(settle());

    const auto holder = hold(panel->GetModelIds().front());
    REQUIRE(panel->exportFile(output)); // queued behind the holder
    const auto cards = panel->tasks().snapshots();
    REQUIRE(cards.size() == 2);
    panel->tasks().cancel(cards[1].id);
    panel->tasks().cancel(holder);
    REQUIRE(settle());

    CHECK_FALSE(std::filesystem::exists(output));
    CHECK_FALSE(tests::support::hasTemporaryFiles(tmp.path()));
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: a move on a busy model waits for it", "[integration][ui][viewer][tasks]")
{
    panel->loadFile(kCube);
    REQUIRE(settle());

    const auto holder = hold(panel->GetModelIds().front());
    panel->setTransform(0, 5, 0, 0, 0, 0, 0);
    CHECK(panel->GetTransform(0).moveX == 0.0); // queued behind the holder
    CHECK(panel->tasks().pending() == 2);

    panel->tasks().cancel(holder); // the next task on the model goes ahead
    REQUIRE(settle());
    CHECK(panel->GetTransform(0).moveX == 5.0);
}

TEST_CASE("ModelViewerPanel: destroying the panel with a running task is safe", "[integration][ui][viewer][tasks]")
{
    ScopedFrame<> frame(nullptr, wxID_ANY, "Busy viewer", wxDefaultPosition, wxSize(320, 240));
    auto*         panel = new ModelViewerPanel(frame.get());
    auto*         sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(panel, 1, wxEXPAND);
    frame->SetSizer(sizer);
    frame.showAndWait();

    REQUIRE(panel->loadFile(kCube));
    CHECK(panel->tasks().pending() == 1);
    // ScopedFrame destroys the frame here, with the import possibly still running.
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: exporting right after opening waits for the import", "[integration][ui][viewer][tasks]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "out.stl";
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(panel->exportFile(output)); // the cube is still being read
    REQUIRE(settle());
    CHECK(std::filesystem::exists(output));
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: dragging the gizmo of a busy model previews, then queues", "[integration][ui][viewer][tasks]")
{
    panel->loadFile(kCube);
    REQUIRE(settle());
    const auto holder = hold(panel->GetModelIds().front());

    panel->SimulateGizmoDrag(0, ModelViewerPanel::TransformValues{5, 0, 0, 0, 0, 0});
    CHECK(panel->GetTransform(0).moveX == 5.0); // the toolbar follows the preview
    CHECK(panel->tasks().pending() == 1);       // nothing queued while dragging

    panel->SimulateGizmoRelease();
    CHECK(panel->GetTransform(0).moveX == 0.0); // back where the scene has it...
    CHECK(panel->tasks().pending() == 2);       // ...with the move queued

    panel->tasks().cancel(holder);
    REQUIRE(settle());
    CHECK(panel->GetTransform(0).moveX == 5.0);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: losing the mouse mid-drag ends the drag", "[integration][ui][viewer][tasks]")
{
    auto* canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    panel->loadFile(kCube);
    REQUIRE(settle());
    const auto holder = hold(panel->GetModelIds().front());

    panel->SimulateGizmoDrag(0, ModelViewerPanel::TransformValues{5, 0, 0, 0, 0, 0});
    wxMouseCaptureLostEvent lost(canvas->GetId());
    lost.SetEventObject(canvas);
    canvas->HandleWindowEvent(lost);
    CHECK(panel->tasks().pending() == 2); // the previewed move was queued
    CHECK(panel->GetTransform(0).moveX == 0.0);

    panel->tasks().cancel(holder);
    REQUIRE(settle());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: an import finishing mid-orbit leaves the camera alone", "[integration][ui][viewer][tasks]")
{
    auto* canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    panel->loadFile(kCube);
    REQUIRE(settle());
    sendMouse(canvas, wxEVT_MIDDLE_DOWN, 100, 100); // pan the camera away from the fitted view
    sendMouse(canvas, wxEVT_MOTION, 160, 140);
    sendMouse(canvas, wxEVT_MIDDLE_UP, 160, 140);
    const auto panned = panel->CameraEye();

    sendMouse(canvas, wxEVT_LEFT_DOWN, 100, 100); // orbiting...
    panel->loadFile(kCube);
    REQUIRE(settle()); // ...when the import lands
    CHECK(panel->CameraEye() == panned);
    sendMouse(canvas, wxEVT_LEFT_UP, 100, 100);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: importing never converts to a B-Rep", "[integration][ui][viewer][tasks]")
{
    std::vector<std::string> names;
    panel->tasks().setListener([&](const application::tasks::TaskSnapshot& snapshot, bool removed) {
        if (!removed)
            names.push_back(snapshot.name);
    });
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    panel->setTransform(0, 1, 0, 0, 0, 0, 0);
    tests::support::TempDir tmp;
    REQUIRE(panel->exportFile(tmp.path() / "out.stl"));
    REQUIRE(settle());

    for (const auto& name : names)
        CHECK(name.rfind("Convert", 0) != 0);
    CHECK(panel->GetModelIds().size() == 1);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: imports in the mesh precision setting", "[integration][ui][viewer][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    settings.set(ui::framework::occt::kMeshPrecisionSetting, "double");
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    REQUIRE(panel->Scene().size() == 1);
    CHECK(panel->Scene().models()[0].shape.precision() == geometry::occt::Shape::Precision::Double);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: computes the B-Rep on import when set", "[integration][ui][viewer][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    CHECK(panel->Scene().brep(panel->GetModelIds()[0]) == nullptr); // off by default

    std::size_t asked = 0;
    panel->SetConfirmConvertLoaded([&](ModelViewerPanel::LoadedModelsQuestion, std::size_t models) {
        asked = models;
        return false; // only new imports
    });
    settings.set(ui::framework::occt::kComputeBRepSetting, "1");
    CHECK(asked == 1);
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    REQUIRE(panel->Scene().size() == 2);
    CHECK(panel->Scene().brep(panel->GetModelIds()[0]) == nullptr);
    CHECK(panel->Scene().brep(panel->GetModelIds()[1]) != nullptr);
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: changing the precision offers to convert loaded models", "[integration][ui][viewer][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    settings.reset(ui::framework::occt::kMeshPrecisionSetting); // the registry outlives each section's run
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    const auto& model = panel->Scene().models()[0];

    std::size_t asked   = 0;
    bool        convert = false;
    panel->SetConfirmConvertLoaded([&](ModelViewerPanel::LoadedModelsQuestion question, std::size_t models) {
        CHECK(question == ModelViewerPanel::LoadedModelsQuestion::Precision);
        asked = models;
        return convert;
    });

    SECTION("kept as is")
    {
        settings.set(ui::framework::occt::kMeshPrecisionSetting, "double");
        REQUIRE(settle());
        CHECK(asked == 1);
        CHECK(model.shape.precision() == geometry::occt::Shape::Precision::Single);
    }
    SECTION("converted, placement kept")
    {
        panel->setTransform(0, 4.0, 0, 0, 0, 0, 0);
        convert = true;
        settings.set(ui::framework::occt::kMeshPrecisionSetting, "double");
        REQUIRE(settle());
        CHECK(asked == 1);
        CHECK(panel->Scene().models()[0].shape.precision() == geometry::occt::Shape::Precision::Double);
        CHECK(panel->GetTransform(0).moveX == 4.0);

        asked = 0;
        settings.set(ui::framework::occt::kMeshPrecisionSetting, "double"); // unchanged: nothing asked
        CHECK(asked == 0);
    }
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: turning B-Reps on import on offers to convert loaded models", "[integration][ui][viewer][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    settings.reset(ui::framework::occt::kComputeBRepSetting); // the registry outlives each section's run
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(panel->loadFile(kCube));
    REQUIRE(settle());
    const auto ids = panel->GetModelIds();
    REQUIRE(ids.size() == 2);

    std::size_t asked   = 0;
    bool        convert = false;
    panel->SetConfirmConvertLoaded([&](ModelViewerPanel::LoadedModelsQuestion question, std::size_t models) {
        CHECK(question == ModelViewerPanel::LoadedModelsQuestion::BRep);
        asked = models;
        return convert;
    });

    SECTION("kept as is")
    {
        settings.set(ui::framework::occt::kComputeBRepSetting, "1");
        REQUIRE(settle());
        CHECK(asked == 2);
        CHECK(panel->Scene().brep(ids[0]) == nullptr);
    }
    SECTION("converted; models that have one aren't asked about again")
    {
        convert = true;
        settings.set(ui::framework::occt::kComputeBRepSetting, "1");
        REQUIRE(settle());
        CHECK(asked == 2);
        CHECK(panel->Scene().brep(ids[0]) != nullptr);
        CHECK(panel->Scene().brep(ids[1]) != nullptr);

        asked = 0;
        settings.set(ui::framework::occt::kComputeBRepSetting, "0"); // turning it off asks nothing
        settings.set(ui::framework::occt::kComputeBRepSetting, "1"); // nothing left to convert
        CHECK(asked == 0);
    }
}

namespace
{
    // Two unit cubes side by side (the second moved to x=3), framed in the view.
    class SelectionFixture : public ViewerFixture
    {
      public:
        SelectionFixture()
        {
            panel->loadFile(kCube);
            REQUIRE(settle());
            panel->loadFile(kCube);
            REQUIRE(settle());
            panel->setTransform(1, 3, 0, 0, 0, 0, 0);
            REQUIRE(tests::support::pumpUntil([this] { return panel->PickDataReady(); }));
            panel->FitAll();
            panel->SetOnSelectionChanged([this] { ++selectionChanges; });
            panel->SetOnBackgroundClicked([this] { ++backgroundClicks; });
        }

        // Where the centre of model i's unit cube shows on the canvas.
        wxPoint centreOf(int i) const
        {
            const auto t = panel->GetTransform(i);
            return panel->ProjectToCanvas(gp_Pnt(t.moveX + 0.5, t.moveY + 0.5, t.moveZ + 0.5));
        }

        int selectionChanges = 0;
        int backgroundClicks = 0;
    };

    const wxPoint kCorner(2, 2);
} // namespace

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: picking finds the model under a point", "[integration][ui][viewer][selection]")
{
    CHECK(panel->PickModelAt(centreOf(0)) == 0);
    CHECK(panel->PickModelAt(centreOf(1)) == 1);
    CHECK(panel->PickModelAt(kCorner) == -1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: clicks select, replace, toggle and clear", "[integration][ui][viewer][selection]")
{
    panel->SimulateClick(centreOf(0), false);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});
    CHECK(panel->IsHighlighted(0));
    CHECK_FALSE(panel->IsHighlighted(1));

    panel->SimulateClick(centreOf(1), false);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{1});
    CHECK_FALSE(panel->IsHighlighted(0));
    CHECK(panel->IsHighlighted(1));

    panel->SimulateClick(centreOf(0), true);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{1, 0});
    panel->SimulateClick(centreOf(1), true);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});

    panel->SimulateClick(kCorner, true); // toggle on nothing: kept
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});
    CHECK(backgroundClicks == 0);

    panel->SimulateClick(kCorner, false);
    CHECK(panel->GetSelectedIndices().empty());
    CHECK_FALSE(panel->IsHighlighted(0));
    CHECK(backgroundClicks == 1);
    CHECK(selectionChanges == 5);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: click threshold", "[integration][ui][viewer][selection]")
{
    auto* canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    REQUIRE(canvas != nullptr);
    const wxPoint c0 = centreOf(0);

    // A little jitter is still a click.
    sendMouse(canvas, wxEVT_LEFT_DOWN, c0.x, c0.y);
    sendMouse(canvas, wxEVT_MOTION, c0.x + 2, c0.y + 2);
    sendMouse(canvas, wxEVT_LEFT_UP, c0.x + 2, c0.y + 2);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});

    // Further is an orbit: the selection stays.
    panel->ClearSelection();
    sendMouse(canvas, wxEVT_LEFT_DOWN, c0.x, c0.y);
    sendMouse(canvas, wxEVT_MOTION, c0.x + 10, c0.y);
    sendMouse(canvas, wxEVT_LEFT_UP, c0.x + 10, c0.y);
    CHECK(panel->GetSelectedIndices().empty());
    tests::support::pumpEvents();
    CHECK_FALSE(canvas->HasCapture());
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: hidden models can't be picked and leave the selection when hidden", "[integration][ui][viewer][selection]")
{
    const wxPoint c0 = centreOf(0);
    panel->SelectOnly(0);
    panel->SetModelVisibility(0, false);
    CHECK(panel->GetSelectedIndices().empty());
    CHECK(panel->PickModelAt(c0) == -1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: hidden models can be selected, and stay hidden", "[integration][ui][viewer][selection]")
{
    panel->SetModelVisibility(0, false);
    panel->SelectOnly(0);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});
    panel->ToggleSelected(1);
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0, 1});
    panel->Select({1, 0});
    CHECK(panel->GetSelectedIndices() == std::vector<int>{1, 0});
    CHECK(panel->GetModelVisibility() == std::vector<bool>{false, true});

    panel->SetModelVisibility(0, true); // shown again: still selected, highlighted
    CHECK(panel->GetSelectedIndices() == std::vector<int>{1, 0});
    CHECK(panel->IsHighlighted(0));
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: DuplicateModels adds copies in place", "[integration][ui][viewer]")
{
    panel->SetModelVisibility(0, false);
    const auto added = panel->DuplicateModels({1, 0}, 2);
    CHECK(added == std::vector<int>{2, 3, 4, 5});
    CHECK(panel->GetModelNames() == std::vector<wxString>{"cube.stl", "cube.stl (2)", "cube.stl (3)", "cube.stl (4)",
                                                          "cube.stl (5)", "cube.stl (6)"});
    // Copies of model 1 (moved to x = 3) then of model 0, twice; hidden copies of the hidden one.
    CHECK(panel->GetTransform(2).moveX == 3);
    CHECK(panel->GetTransform(3).moveX == 0);
    CHECK(panel->GetTransform(4).moveX == 3);
    CHECK(panel->GetModelVisibility() == std::vector<bool>{false, true, true, false, true, false});

    CHECK(panel->DuplicateModels({}, 3).empty());
    CHECK(panel->DuplicateModels({0}, 0).empty());
    CHECK(panel->DuplicateModels({9}, 1).empty()); // no such model
    CHECK(panel->GetModelNames().size() == 6);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: Select picks several models at once, skipping unknown ones", "[integration][ui][viewer][selection]")
{
    int changes = 0;
    panel->SetOnSelectionChanged([&] { ++changes; });
    panel->Select({1, 0});
    CHECK(panel->GetSelectedIndices() == std::vector<int>{1, 0});
    CHECK(panel->IsHighlighted(0));
    CHECK(panel->IsHighlighted(1));
    CHECK(changes == 1);

    changes = 0;
    panel->Select({7}); // no such model
    CHECK(panel->GetSelectedIndices().empty());
    CHECK(changes == 1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: removed models leave the selection", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->RemoveModel(0);
    REQUIRE(settle());
    CHECK(panel->GetSelectedIndices() == std::vector<int>{0}); // the former model 1
    CHECK(panel->IsHighlighted(0));
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: highlight survives a re-display", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->FocusModel(1); // erases and re-displays every model

    auto& settings = platform::settings::SettingsRegistry::getInstance();
    settings.reset(ui::framework::occt::kMeshPrecisionSetting);
    panel->SetConfirmConvertLoaded([](ModelViewerPanel::LoadedModelsQuestion, std::size_t) { return true; });
    settings.set(ui::framework::occt::kMeshPrecisionSetting, "double"); // swaps each presentation's shape
    REQUIRE(settle());
    settings.reset(ui::framework::occt::kMeshPrecisionSetting);
    REQUIRE(settle());

    CHECK(panel->IsHighlighted(0));
    CHECK_FALSE(panel->IsHighlighted(1));
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: gizmo hit wins over model pick", "[integration][ui][viewer][selection]")
{
    // Put model 1 right behind the middle of the gizmo's X arrow on model 0.
    const wxPoint origin   = panel->ProjectToCanvas(gp_Pnt(0, 0, 0));
    const wxPoint unitX    = panel->ProjectToCanvas(gp_Pnt(1, 0, 0));
    const double  pxPerMm  = std::hypot(unitX.x - origin.x, unitX.y - origin.y);
    const double  onArrowX = 40.0 / pxPerMm; // the arrow is 80 px long
    panel->setTransform(1, onArrowX - 0.5, -0.5, -0.5, 0, 0, 0);
    const wxPoint target = panel->ProjectToCanvas(gp_Pnt(onArrowX, 0, 0));
    REQUIRE(panel->PickModelAt(target) == 1);

    panel->SelectOnly(0);
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Move);
    panel->SimulateClick(target, false);

    CHECK(panel->GetSelectedIndices() == std::vector<int>{0});
    CHECK(backgroundClicks == 0);
}

namespace
{
    constexpr int kGroup = ModelViewerPanel::kSelectionTarget;

    void requireMove(const ModelViewerPanel::TransformValues& t, double x, double y, double z)
    {
        CHECK_THAT(t.moveX, Catch::Matchers::WithinAbs(x, 1e-6));
        CHECK_THAT(t.moveY, Catch::Matchers::WithinAbs(y, 1e-6));
        CHECK_THAT(t.moveZ, Catch::Matchers::WithinAbs(z, 1e-6));
    }

    void requireSamePlacement(const ModelViewerPanel::TransformValues& actual, const ModelViewerPanel::TransformValues& expected)
    {
        for (const gp_Pnt& p : {gp_Pnt(0, 0, 0), gp_Pnt(1, 0, 0), gp_Pnt(0, 1, 0), gp_Pnt(0, 0, 1)})
        {
            const gp_Pnt a = p.Transformed(geometry::occt::toTrsf(actual));
            const gp_Pnt e = p.Transformed(geometry::occt::toTrsf(expected));
            CHECK(a.Distance(e) < 1e-6);
        }
    }
} // namespace

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: group move", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->setTransform(kGroup, 10, 0, 0, 0, 0, 0);
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 10, 0, 0);
    requireMove(panel->GetTransform(1), 13, 0, 0);
    CHECK(panel->GetTransform(kGroup) == ModelViewerPanel::TransformValues{10, 0, 0, 0, 0, 0});
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: group rotate around the shared centre", "[integration][ui][viewer][selection]")
{
    // Boxes [0,1]^3 and [3,4]x[0,1]^2: centre (2, 0.5, 0.5).
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->setTransform(kGroup, 0, 0, 0, 0, 0, 180);
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 4, 1, 0);
    requireMove(panel->GetTransform(1), 1, 1, 0);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: group offsets combine and survive tool switches", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->SetGizmoState(kGroup, ModelViewerPanel::GizmoTool::Rotate);
    panel->setTransform(kGroup, 0, 0, 0, 0, 0, 90);
    REQUIRE(settle());
    panel->SetGizmoState(kGroup, ModelViewerPanel::GizmoTool::Move);
    panel->setTransform(kGroup, 5, 0, 0, 0, 0, 90);
    REQUIRE(settle());

    const gp_Pnt                            pivot(2, 0.5, 0.5);
    const ModelViewerPanel::TransformValues offset{5, 0, 0, 0, 0, 90};
    requireSamePlacement(panel->GetTransform(0), geometry::occt::placeInGroup({}, pivot, offset));
    requireSamePlacement(panel->GetTransform(1), geometry::occt::placeInGroup({3, 0, 0, 0, 0, 0}, pivot, offset));
    CHECK(panel->GetTransform(kGroup) == offset);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: a selection change resets the group offset", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->setTransform(kGroup, 10, 0, 0, 0, 0, 0);
    REQUIRE(settle());

    panel->ToggleSelected(1);
    panel->ToggleSelected(1);
    CHECK(panel->GetTransform(kGroup) == ModelViewerPanel::TransformValues{});
    requireMove(panel->GetTransform(0), 10, 0, 0); // the models stay where they went

    panel->setTransform(kGroup, 1, 0, 0, 0, 0, 0); // from the new starting point
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 11, 0, 0);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: a group move on a busy model is queued", "[integration][ui][viewer][selection][tasks]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    const auto holder = hold(panel->GetModelIds()[1]);

    panel->setTransform(kGroup, 10, 0, 0, 0, 0, 0);
    tests::support::pumpEvents();
    requireMove(panel->GetTransform(1), 3, 0, 0);

    panel->tasks().cancel(holder);
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 10, 0, 0);
    requireMove(panel->GetTransform(1), 13, 0, 0);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: dragging the group gizmo", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    panel->SetGizmoState(kGroup, ModelViewerPanel::GizmoTool::Move);

    panel->SimulateGizmoDrag(kGroup, ModelViewerPanel::TransformValues{3, 0, 0, 0, 0, 0});
    panel->SimulateGizmoRelease();
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 3, 0, 0);
    requireMove(panel->GetTransform(1), 6, 0, 0);
    CHECK(panel->GetTransform(kGroup) == ModelViewerPanel::TransformValues{3, 0, 0, 0, 0, 0});
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: the group target needs two selected models", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    panel->setTransform(kGroup, 10, 0, 0, 0, 0, 0);
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 0, 0, 0);
    CHECK(panel->GetTransform(kGroup) == ModelViewerPanel::TransformValues{});
    panel->SetGizmoState(kGroup, ModelViewerPanel::GizmoTool::Move); // nothing to show; harmless
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: a new group starts from moves still queued", "[integration][ui][viewer][selection][tasks]")
{
    panel->SelectOnly(0);
    panel->ToggleSelected(1);
    const auto holder = hold(panel->GetModelIds()[1]);
    panel->setTransform(kGroup, 10, 0, 0, 0, 0, 0); // model 1's move waits

    panel->ToggleSelected(1); // a new selection: a new starting point
    panel->ToggleSelected(1);
    panel->setTransform(kGroup, 0, 5, 0, 0, 0, 0);

    panel->tasks().cancel(holder);
    REQUIRE(settle());
    requireMove(panel->GetTransform(0), 10, 5, 0);
    requireMove(panel->GetTransform(1), 13, 5, 0); // the first move isn't undone
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: pressing next to the gizmo leaves no hover highlight", "[integration][ui][viewer][selection]")
{
    auto*         canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    const wxPoint c1     = centreOf(1);
    REQUIRE(panel->PickModelAt(c1) == 1); // model 1 is pickable from now on
    panel->SetGizmoState(0, ModelViewerPanel::GizmoTool::Move);

    sendMouse(canvas, wxEVT_LEFT_DOWN, c1.x, c1.y); // misses the gizmo: an orbit starts
    CHECK_FALSE(panel->HasHoverHighlight());
    sendMouse(canvas, wxEVT_LEFT_UP, c1.x, c1.y);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: the model in front is picked", "[integration][ui][viewer][selection]")
{
    // The view looks along -(1,-1,1): move model 1 straight behind model 0, then in front.
    panel->setTransform(1, -3, 3, -3, 0, 0, 0);
    REQUIRE(settle());
    CHECK(panel->PickModelAt(centreOf(0)) == 0);
    panel->setTransform(1, 3, -3, 3, 0, 0, 0);
    REQUIRE(settle());
    CHECK(panel->PickModelAt(centreOf(0)) == 1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: click data is rebuilt for a new mesh", "[integration][ui][viewer][selection][settings]")
{
    auto& settings = platform::settings::SettingsRegistry::getInstance();
    settings.reset(ui::framework::occt::kMeshPrecisionSetting);
    panel->SetConfirmConvertLoaded([](ModelViewerPanel::LoadedModelsQuestion, std::size_t) { return true; });
    settings.set(ui::framework::occt::kMeshPrecisionSetting, "double");
    REQUIRE(settle());
    REQUIRE(tests::support::pumpUntil([this] { return panel->PickDataReady(); }));
    settings.reset(ui::framework::occt::kMeshPrecisionSetting);
    REQUIRE(settle());
    REQUIRE(tests::support::pumpUntil([this] { return panel->PickDataReady(); }));

    CHECK(panel->PickModelAt(centreOf(0)) == 0);
    CHECK(panel->PickModelAt(centreOf(1)) == 1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: models are picked again after hiding, showing and focusing", "[integration][ui][viewer][selection]")
{
    REQUIRE(panel->PickModelAt(centreOf(0)) == 0);
    panel->SetModelVisibility(0, false);
    panel->SetModelVisibility(0, true);
    CHECK(panel->PickModelAt(centreOf(0)) == 0);
    panel->FocusModel(1);
    panel->FitAll();
    CHECK(panel->PickModelAt(centreOf(0)) == 0);
    CHECK(panel->PickModelAt(centreOf(1)) == 1);
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: a release far from the press is not a click", "[integration][ui][viewer][selection]")
{
    auto*         canvas = tests::support::findDescendant<wxGLCanvas>(panel);
    const wxPoint c0     = centreOf(0);
    // A fast flick: no motion event between press and release.
    sendMouse(canvas, wxEVT_LEFT_DOWN, c0.x - 20, c0.y);
    sendMouse(canvas, wxEVT_LEFT_UP, c0.x, c0.y);
    CHECK(panel->GetSelectedIndices().empty());
}

TEST_CASE_METHOD(SelectionFixture, "ModelViewerPanel: on a removal listeners hear of the new models before the new selection", "[integration][ui][viewer][selection]")
{
    panel->SelectOnly(0);
    std::vector<std::string> heard;
    std::size_t              modelsWhenSelectionChanged = 0;
    panel->SetOnModelStateChanged([&](bool) { heard.emplace_back("models"); });
    panel->SetOnSelectionChanged([&] {
        heard.emplace_back("selection");
        modelsWhenSelectionChanged = panel->GetModelNames().size();
    });
    panel->RemoveModel(0);
    REQUIRE(settle());
    CHECK(heard == std::vector<std::string>{"models", "selection"});
    CHECK(modelsWhenSelectionChanged == 1);
}
