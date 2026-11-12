#include "application/tasks/task.hpp"
#include "geometry/occt/shape.hpp"
#include "platform/logger/logger.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <chrono>
#include <filesystem>
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
