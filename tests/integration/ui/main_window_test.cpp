#include "application/tasks/task.hpp"
#include "platform/settings/settings_registry.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registration.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/panels/settings_panel/settings_panel.hpp"
#include "ui/panels/side_dock/side_dock.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"
#include "ui/windows/main_window/main_window.hpp"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <filesystem>
#include <gp_Pnt.hxx>
#include <thread>
#include <wx/combobox.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/textentry.h>
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
            // A real dialog would block the test until someone clicks it.
            viewer->SetConfirmConvertLoaded([](ModelViewerPanel::LoadedModelsQuestion, std::size_t) {
                FAIL_CHECK("unexpected \"convert loaded models?\" dialog");
                return false;
            });
        }

        // Waits until every task the viewer started is over.
        bool settle()
        {
            return tests::support::pumpUntil([this] { return viewer->tasks().idle(); });
        }

        // A task that holds the first model until cancelled.
        void hold()
        {
            application::tasks::Task task;
            task.name      = "hold";
            task.operation = "test.hold";
            task.writes    = {viewer->GetModelIds().front()};
            task.work      = [](application::tasks::TaskContext& context) {
                while (!context.cancelled())
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
            };
            task.apply = [] { };
            viewer->tasks().submit(std::move(task));
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
    REQUIRE(settle());

    CHECK(moveButton->IsEnabled());
    REQUIRE(combo->GetCount() == 2);
    CHECK(combo->GetString(1) == "cube.stl");
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: editing a toolbar field moves the selected model", "[integration][ui][main_window]")
{
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());

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
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
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
    REQUIRE(settle());
    auto* moveButton = findDescendant<wxToggleButton>(toolbar);
    REQUIRE(moveButton->IsEnabled());

    sendMenuCommand(window.get(), ids::file::kNew);

    CHECK_FALSE(viewer->hasModel());
    CHECK_FALSE(moveButton->IsEnabled());
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: the tasks panel shows while a task runs", "[integration][ui][main_window][tasks]")
{
    auto* panel = findDescendant<ui::panels::TasksPanel>(window.get());
    REQUIRE(panel != nullptr);
    CHECK_FALSE(panel->IsShown());

    REQUIRE(viewer->loadFile(kCube));
    CHECK(panel->IsShown());
    REQUIRE(settle());
    CHECK_FALSE(panel->IsShown());
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: the tasks panel opened by hand stays open", "[integration][ui][main_window][tasks]")
{
    auto* dock  = findDescendant<ui::panels::SideDock>(window.get());
    auto* panel = findDescendant<ui::panels::TasksPanel>(window.get());
    REQUIRE(dock != nullptr);
    REQUIRE(panel != nullptr);

    dock->Open(panel);
    CHECK(panel->IsShown());
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    tests::support::pumpEvents();
    CHECK(panel->IsShown());
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: closing with pending tasks asks first", "[integration][ui][main_window][tasks]")
{
    tests::support::TempDir tmp;
    const auto              output = tmp.path() / "out.stl";
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    hold();
    REQUIRE(viewer->exportFile(output)); // queued behind the holder

    std::size_t asked = 0;
    window->SetConfirmQuit([&](std::size_t pending) {
        asked = pending;
        return false; // Keep working
    });
    window->Close();
    CHECK(asked == 2);
    CHECK(window->IsShown());
    CHECK(viewer->tasks().pending() == 2);

    window->SetConfirmQuit([](std::size_t) { return true; }); // Quit
    window->Close();
    // Not pumped: the frame is only deleted at the next idle, by ScopedFrame.
    CHECK(viewer->tasks().pending() == 0);
    CHECK_FALSE(std::filesystem::exists(output));
    CHECK_FALSE(tests::support::hasTemporaryFiles(tmp.path()));
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: tasks don't open the dock when auto-opening is off", "[integration][ui][main_window][tasks][settings]")
{
    auto* panel = findDescendant<ui::panels::TasksPanel>(window.get());
    REQUIRE(panel != nullptr);
    platform::settings::SettingsRegistry::getInstance().set(ui::windows::kTasksAutoOpenSetting, "0");

    REQUIRE(viewer->loadFile(kCube));
    CHECK_FALSE(panel->IsShown());
    REQUIRE(settle());
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: the dock has a settings page listing every setting", "[integration][ui][main_window][settings]")
{
    auto* dock     = findDescendant<ui::panels::SideDock>(window.get());
    auto* settings = findDescendant<ui::panels::SettingsPanel>(window.get());
    REQUIRE(dock != nullptr);
    REQUIRE(settings != nullptr);
    REQUIRE(dock->GetButton(settings) != nullptr);

    dock->Open(settings);
    tests::support::pumpEvents();
    // Declared by the viewer, the window and the shortcuts (bound after the panels).
    auto& registry = platform::settings::SettingsRegistry::getInstance();
    for (const char* key : {ui::framework::occt::kMeshPrecisionSetting, ui::framework::occt::kComputeBRepSetting,
                            ui::windows::kTasksAutoOpenSetting, "keys.save"})
    {
        CAPTURE(key);
        CHECK(registry.find(key) != nullptr);
    }
    CHECK(findDescendant<ui::widgets::KeyBindButton>(settings) != nullptr);
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: the File menu shows the current shortcuts", "[integration][ui][main_window][settings]")
{
    wxMenuItem* save = window->GetMenuBar()->FindItem(ids::file::kSave);
    REQUIRE(save != nullptr);
    CHECK(save->GetItemLabel().AfterFirst('\t') == "Ctrl+S");

    platform::settings::SettingsRegistry::getInstance().set("keys.save", "Ctrl+Shift+E");
    CHECK(save->GetItemLabel().AfterFirst('\t') == "Ctrl+Shift+E");
}

namespace
{
    // Two cubes loaded, and the delete dialog replaced by a recorder.
    class SelectionWindowFixture : public MainWindowFixture
    {
      public:
        SelectionWindowFixture()
        {
            REQUIRE(viewer->loadFile(kCube));
            REQUIRE(settle());
            REQUIRE(viewer->loadFile(kCube));
            REQUIRE(settle());
            platform::settings::SettingsRegistry::getInstance().reset(ui::windows::kConfirmDeleteSetting);
            window->SetConfirmDelete([this](const wxString& message) {
                asked.push_back(message);
                return answer;
            });
            combo = findDescendant<wxComboBox>(toolbar);
            REQUIRE(combo != nullptr);
        }

        ~SelectionWindowFixture()
        {
            platform::settings::SettingsRegistry::getInstance().reset(ui::windows::kConfirmDeleteSetting);
        }

        void pick(int entry)
        {
            combo->SetSelection(entry);
            wxCommandEvent event(wxEVT_COMBOBOX, combo->GetId());
            event.SetEventObject(combo);
            combo->HandleWindowEvent(event);
        }

        void openMove()
        {
            auto* button = findDescendant<wxToggleButton>(toolbar);
            button->SetValue(true);
            wxCommandEvent event(wxEVT_TOGGLEBUTTON, button->GetId());
            event.SetEventObject(button);
            event.SetInt(1);
            button->HandleWindowEvent(event);
        }

        wxComboBox*           combo = nullptr;
        std::vector<wxString> asked;
        bool                  answer = true;
    };
} // namespace

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the selection drives the target dropdown", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(1);
    CHECK(toolbar->GetTargetIndex() == 1);
    viewer->ToggleSelected(0);
    CHECK(toolbar->GetTargetIndex() == ModelViewerPanel::kSelectionTarget);
    CHECK(combo->GetCount() == 4); // All models, 2 cubes, "2 selected"
    viewer->ClearSelection();
    CHECK(toolbar->GetTargetIndex() == -1);
    CHECK(combo->GetCount() == 3);
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the target dropdown drives the selection", "[integration][ui][main_window][selection]")
{
    pick(1);
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{0});
    pick(0);
    CHECK(viewer->GetSelectedIndices().empty());
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: a click on empty space closes the open panel", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(0);
    openMove();
    REQUIRE(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::Move);

    viewer->SimulateClick(wxPoint(2, 2), false);
    CHECK(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::None);
    CHECK(viewer->GetSelectedIndices().empty());
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: Move opens on the selected model", "[integration][ui][main_window][selection]")
{
    viewer->setTransform(1, 7, 0, 0, 0, 0, 0);
    REQUIRE(settle());
    viewer->SelectOnly(1);
    openMove();
    CHECK(toolbar->GetTargetIndex() == 1);
    CHECK(toolbar->GetMoveX()->GetValue() == 7.0);

    viewer->ToggleSelected(0); // a group: fields start from zero
    CHECK(toolbar->GetMoveX()->GetValue() == 0.0);
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: deleting asks first", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(0);
    answer = false;
    window->DeleteModels(viewer->GetSelectedIndices());
    REQUIRE(settle());
    CHECK(viewer->GetModelNames().size() == 2);
    CHECK(asked == std::vector<wxString>{"Delete \"cube.stl\"?"});
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{0}); // nothing changed

    answer = true;
    window->DeleteModels(viewer->GetSelectedIndices());
    REQUIRE(settle());
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl (2)"});
    CHECK(viewer->GetSelectedIndices().empty());
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: deleting without confirmation", "[integration][ui][main_window][selection]")
{
    platform::settings::SettingsRegistry::getInstance().set(ui::windows::kConfirmDeleteSetting, "0");
    window->DeleteModels({1});
    REQUIRE(settle());
    CHECK(asked.empty());
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl"});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: deleting several models", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(0);
    viewer->ToggleSelected(1);
    window->DeleteModels(viewer->GetSelectedIndices());
    REQUIRE(settle());
    CHECK(asked == std::vector<wxString>{"Delete 2 models?"});
    CHECK_FALSE(viewer->hasModel());
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: nothing selected, nothing deleted", "[integration][ui][main_window][selection]")
{
    window->DeleteModels({});
    CHECK(asked.empty());
    CHECK(viewer->GetModelNames().size() == 2);
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the delete key deletes the selection", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(1);
    wxKeyEvent key(wxEVT_CHAR_HOOK);
#if defined(__WXOSX__)
    key.m_keyCode = WXK_BACK;
#else
    key.m_keyCode = WXK_DELETE;
#endif
    key.SetEventObject(window.get());
    window->ProcessWindowEvent(key);
    REQUIRE(settle());
    CHECK(asked.size() == 1);
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl"});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: a removal during the delete dialog doesn't change what is deleted", "[integration][ui][main_window][selection]")
{
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    window->SetConfirmDelete([this](const wxString&) {
        viewer->RemoveModel(0); // an earlier removal lands while the dialog is up
        return true;
    });
    window->DeleteModels({1}); // "cube.stl (2)"
    REQUIRE(settle());
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl (3)"});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: picking a hidden model in the dropdown selects nothing", "[integration][ui][main_window][selection]")
{
    viewer->SetModelVisibility(1, false);
    viewer->SelectOnly(0);
    pick(2); // the hidden model
    CHECK(viewer->GetSelectedIndices().empty());
    CHECK(toolbar->GetTargetIndex() == -1); // the dropdown says what is selected
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: after clicking a model, the delete key deletes it even from a field", "[integration][ui][main_window][selection]")
{
    viewer->setTransform(1, 3, 0, 0, 0, 0, 0);
    REQUIRE(settle());
    REQUIRE(tests::support::pumpUntil([this] { return viewer->PickDataReady(); }));
    viewer->FitAll();
    openMove();
    toolbar->GetMoveX()->SetFocus();
    tests::support::pumpEvents();
    REQUIRE(dynamic_cast<wxTextEntry*>(wxWindow::FindFocus()) != nullptr);

    auto*         canvas = findDescendant<wxGLCanvas>(viewer);
    const wxPoint c1     = viewer->ProjectToCanvas(gp_Pnt(3.5, 0.5, 0.5));
    for (const wxEventType type : {wxEVT_LEFT_DOWN, wxEVT_LEFT_UP})
    {
        wxMouseEvent click(type);
        click.SetEventObject(canvas);
        click.SetPosition(c1);
        click.m_x = c1.x;
        click.m_y = c1.y;
        canvas->HandleWindowEvent(click);
    }
    tests::support::pumpEvents();
    REQUIRE(viewer->GetSelectedIndices() == std::vector<int>{1});

    wxKeyEvent key(wxEVT_CHAR_HOOK);
#if defined(__WXOSX__)
    key.m_keyCode = WXK_BACK;
#else
    key.m_keyCode = WXK_DELETE;
#endif
    key.SetEventObject(window.get());
    window->ProcessWindowEvent(key);
    REQUIRE(settle());
    CHECK(asked.size() == 1);
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the delete key works with the target dropdown focused", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(1);
    openMove();
    combo->SetFocus();
    tests::support::pumpEvents();

    wxKeyEvent key(wxEVT_CHAR_HOOK);
#if defined(__WXOSX__)
    key.m_keyCode = WXK_BACK;
#else
    key.m_keyCode = WXK_DELETE;
#endif
    key.SetEventObject(window.get());
    window->ProcessWindowEvent(key);
    REQUIRE(settle());
    CHECK(asked.size() == 1);
}

namespace
{
    // Returns whether the window let the key go on (nothing handled it).
    bool pressEscape(wxWindow* window)
    {
        wxKeyEvent key(wxEVT_CHAR_HOOK);
        key.m_keyCode = WXK_ESCAPE;
        key.SetEventObject(window);
        window->ProcessWindowEvent(key);
        return key.GetSkipped();
    }
} // namespace

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: Escape closes the tool and clears the selection", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(0);
    viewer->ToggleSelected(1);
    openMove();
    REQUIRE(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::Move);

    CHECK_FALSE(pressEscape(window.get()));
    CHECK(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::None);
    CHECK(viewer->GetSelectedIndices().empty());
    CHECK(toolbar->GetTargetIndex() == -1);

    CHECK(pressEscape(window.get())); // nothing left to clear: the key goes on
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: Escape with only a selection or only a tool", "[integration][ui][main_window][selection]")
{
    viewer->SelectOnly(1);
    CHECK_FALSE(pressEscape(window.get()));
    CHECK(viewer->GetSelectedIndices().empty());

    openMove();
    CHECK_FALSE(pressEscape(window.get()));
    CHECK(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::None);
}
