#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registration.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"
#include "ui/windows/main_window/main_window.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <wx/combobox.h>
#include <wx/menu.h>
#include <wx/tglbtn.h>

// The whole main window as Bootstrap assembles it: registries -> menus, toolbar,
// layout and viewer -> MainWindow's wiring between toolbar, viewer and EventManager.

namespace ids = ui::framework::wx::ids;
using tests::support::findDescendant;
using tests::support::ScopedFrame;
using tests::support::sendMenuCommand;
using ui::framework::occt::ModelViewerPanel;
using ui::toolbars::TransformToolbar;

namespace
{
    const std::filesystem::path kCube = std::filesystem::path(SPLINTER3D_TEST_ASSETS_DIR) / "stl" / "ascii" / "cube.stl";

    struct RegisteredFactories
    {
        RegisteredFactories()
        {
            ui::menus::RegisterAllMenus();
            ui::toolbars::RegisterAllToolBars();
            ui::layouts::RegisterAllLayouts();
            ui::panels::RegisterAllPanels();
        }
    };

    class MainWindowFixture : RegisteredFactories
    {
      public:
        MainWindowFixture()
        {
            window.showAndWait();
            viewer  = findDescendant<ModelViewerPanel>(window.get());
            toolbar = findDescendant<TransformToolbar>(window.get());
            REQUIRE(viewer != nullptr);
            REQUIRE(toolbar != nullptr);
        }

        ScopedFrame<ui::windows::MainWindow> window;
        ModelViewerPanel*                    viewer  = nullptr;
        TransformToolbar*                    toolbar = nullptr;
    };
} // namespace

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: File menu exposes the file commands", "[integration][ui][main_window]")
{
    wxMenuBar* menuBar = window->GetMenuBar();
    REQUIRE(menuBar != nullptr);
    REQUIRE(menuBar->FindMenu("File") != wxNOT_FOUND);

    for (int id : {ids::file::kNew, ids::file::kOpen, ids::file::kSave, ids::file::kExit})
    {
        CAPTURE(id);
        CHECK(menuBar->FindItem(id) != nullptr);
    }
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: loading a model enables the transform toolbar", "[integration][ui][main_window]")
{
    auto* moveButton = findDescendant<wxToggleButton>(toolbar);
    auto* combo      = findDescendant<wxComboBox>(toolbar);
    REQUIRE(moveButton != nullptr);
    REQUIRE(combo != nullptr);
    CHECK_FALSE(moveButton->IsEnabled());

    REQUIRE(viewer->loadFile(kCube));

    CHECK(moveButton->IsEnabled());
    REQUIRE(combo->GetCount() == 2);
    CHECK(combo->GetString(1) == "cube.stl");
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: editing a toolbar field moves the selected model", "[integration][ui][main_window]")
{
    viewer->loadFile(kCube);
    viewer->loadFile(kCube);

    // Select the second model, as the user would in the dropdown.
    auto* combo = findDescendant<wxComboBox>(toolbar);
    combo->SetSelection(2);
    wxCommandEvent pick(wxEVT_COMBOBOX, combo->GetId());
    pick.SetEventObject(combo);
    combo->HandleWindowEvent(pick);
    REQUIRE(toolbar->GetTargetIndex() == 1);

    // Type a value in the X field and commit it.
    wxSpinCtrlDouble* moveX = toolbar->GetMoveX();
    moveX->SetValue(42.0);
    wxSpinDoubleEvent edit(wxEVT_SPINCTRLDOUBLE, moveX->GetId(), 42.0);
    edit.SetEventObject(moveX);
    moveX->HandleWindowEvent(edit);

    CHECK(viewer->GetTransform(1).moveX == 42.0);
    CHECK(viewer->GetTransform(0).moveX == 0.0);
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: switching target refreshes the fields", "[integration][ui][main_window]")
{
    viewer->loadFile(kCube);
    viewer->setTransform(0, 7, 8, 9, 10, 20, 30);

    auto* combo = findDescendant<wxComboBox>(toolbar);
    combo->SetSelection(1);
    wxCommandEvent pick(wxEVT_COMBOBOX, combo->GetId());
    pick.SetEventObject(combo);
    combo->HandleWindowEvent(pick);

    CHECK(toolbar->GetMoveX()->GetValue() == 7.0);
    CHECK(toolbar->GetMoveZ()->GetValue() == 9.0);
    CHECK(toolbar->GetRotateY()->GetValue() == 20.0);
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: File > New clears the scene and disables the toolbar", "[integration][ui][main_window]")
{
    viewer->loadFile(kCube);
    auto* moveButton = findDescendant<wxToggleButton>(toolbar);
    REQUIRE(moveButton->IsEnabled());

    sendMenuCommand(window.get(), ids::file::kNew);

    CHECK_FALSE(viewer->hasModel());
    CHECK_FALSE(moveButton->IsEnabled());
}
