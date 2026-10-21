#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>
#include <wx/glcanvas.h>
#include <wx/sizer.h>

// Exercises the real OCCT viewer (OpenGL context, AIS presentations, manipulator)
// inside a shown frame. On headless Linux this needs Xvfb + Mesa (see Tests.py).

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

    CHECK(panel->hasModel());
    CHECK(panel->GetModelNames() == std::vector<wxString>{"cube.stl", "cube.stl (2)"});
    CHECK(states == std::vector<bool>{true, true});
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: a failed load changes nothing", "[integration][ui][viewer]")
{
    CHECK_FALSE(panel->loadFile(kCube.parent_path() / "missing.stl"));

    CHECK_FALSE(panel->hasModel());
    CHECK(states.empty());
}

TEST_CASE_METHOD(ViewerFixture, "ModelViewerPanel: transforms are applied per target", "[integration][ui][viewer]")
{
    panel->loadFile(kCube);
    panel->loadFile(kCube);

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
    CHECK(panel->exportFile(output));
    CHECK(std::filesystem::file_size(output) > 0);

    panel->newFile();
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
