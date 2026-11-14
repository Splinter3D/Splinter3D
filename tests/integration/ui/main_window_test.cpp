#include "application/tasks/task.hpp"
#include "platform/locale/locale_manager.hpp"
#include "platform/settings/settings_registry.hpp"
#include "support/temp_dir.hpp"
#include "support/wx_helpers.hpp"
#include "ui/framework/occt/model_viewer_panel.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registration.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/panels/models_panel/models_panel.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/panels/settings_panel/settings_panel.hpp"
#include "ui/panels/side_dock/side_dock.hpp"
#include "ui/panels/tasks_panel/tasks_panel.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/toolbars/transform_toolbar/transform_toolbar.hpp"
#include "ui/widgets/key_bind_button/key_bind_button.hpp"
#include "ui/windows/main_window/main_window.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <gp_Pnt.hxx>
#include <optional>
#include <thread>
#include <vector>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/glcanvas.h>
#include <wx/listctrl.h>
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

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: the dock has a models page listing the models", "[integration][ui][main_window]")
{
    auto* dock   = findDescendant<ui::panels::SideDock>(window.get());
    auto* models = findDescendant<ui::panels::ModelsPanel>(window.get());
    REQUIRE(dock != nullptr);
    REQUIRE(models != nullptr);
    REQUIRE(dock->GetButton(models) != nullptr);
    auto* list = findDescendant<wxListCtrl>(models);
    REQUIRE(list != nullptr);
    CHECK(list->GetItemCount() == 0);

    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    CHECK(list->GetItemCount() == 1);
}

TEST_CASE_METHOD(MainWindowFixture, "MainWindow: a page's shortcut opens it, and closes it when open", "[integration][ui][main_window][shortcuts]")
{
    auto* dock = findDescendant<ui::panels::SideDock>(window.get());
    REQUIRE(dock != nullptr);
    const auto press = [this](int keyCode, bool shift) {
        wxKeyEvent event(wxEVT_CHAR_HOOK);
        event.m_keyCode = keyCode;
        event.SetControlDown(true);
        event.SetShiftDown(shift);
        event.SetEventObject(window.get());
        window->ProcessWindowEvent(event);
        tests::support::pumpEvents();
    };
    wxWindow* const models   = findDescendant<ui::panels::ModelsPanel>(window.get());
    wxWindow* const tasks    = findDescendant<ui::panels::TasksPanel>(window.get());
    wxWindow* const settings = findDescendant<ui::panels::SettingsPanel>(window.get());

    press('O', true);
    CHECK(dock->CurrentPage() == models);
    press('T', true); // another page's key switches to it
    CHECK(dock->CurrentPage() == tasks);
    press(',', false);
    CHECK(dock->CurrentPage() == settings);
    CHECK(settings->IsShown());
    press(',', false);
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(settings->IsShown());
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
            window->SetConfirmDelete([this](const wxString& title, const wxString& message) {
                titles.push_back(title);
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
        std::vector<wxString> titles;
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

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the models list and the viewer share the selection", "[integration][ui][main_window][selection]")
{
    auto* list = findDescendant<wxListCtrl>(findDescendant<ui::panels::ModelsPanel>(window.get()));
    REQUIRE(list != nullptr);
    REQUIRE(list->GetItemCount() == 2);
    const auto clickRow = [list](int row, bool command) {
        wxRect rect;
        REQUIRE(list->GetItemRect(row, rect));
        wxMouseEvent event(wxEVT_LEFT_DOWN);
        event.m_x = rect.x + 4;
        event.m_y = rect.y + rect.height / 2;
        event.SetControlDown(command);
        event.SetEventObject(list);
        list->ProcessWindowEvent(event);
    };

    clickRow(1, false);
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{1});
    clickRow(0, true);
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{1, 0});
    CHECK(list->GetItemState(0, wxLIST_STATE_SELECTED) != 0);
    CHECK(list->GetItemState(1, wxLIST_STATE_SELECTED) != 0);

    viewer->SelectOnly(0); // from the viewport
    CHECK(list->GetItemState(0, wxLIST_STATE_SELECTED) != 0);
    CHECK(list->GetItemState(1, wxLIST_STATE_SELECTED) == 0);
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
    CHECK(titles == std::vector<wxString>{"Delete model"});
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
    CHECK(titles == std::vector<wxString>{"Delete models"});
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

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: H hides the selected models", "[integration][ui][main_window][selection]")
{
    const auto pressH = [this] {
        wxKeyEvent key(wxEVT_CHAR_HOOK);
        key.m_keyCode = 'H';
        key.SetEventObject(window.get());
        window->ProcessWindowEvent(key);
    };
    REQUIRE(platform::settings::SettingsRegistry::getInstance().get("keys.hide_selection") == "H");
    pressH(); // nothing selected: nothing hidden
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{true, true});

    viewer->SelectOnly(1);
    pressH();
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{true, false});
    CHECK(viewer->GetSelectedIndices().empty());
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the models page's buttons act on the selection", "[integration][ui][main_window][selection]")
{
    auto* models = findDescendant<ui::panels::ModelsPanel>(window.get());
    REQUIRE(models != nullptr);
    const auto press = [models](const wxString& label) {
        for (wxWindow* child : models->GetChildren())
            if (auto* button = dynamic_cast<wxButton*>(child); button != nullptr && button->GetLabel() == label)
            {
                REQUIRE(button->IsEnabled());
                wxCommandEvent event(wxEVT_BUTTON, button->GetId());
                event.SetEventObject(button);
                button->ProcessWindowEvent(event);
                return;
            }
        FAIL("no enabled button " << label.utf8_string());
    };

    viewer->SelectOnly(1);
    press("Show only");
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{false, true});
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{1}); // still selected

    press("Hide");
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{false, false});

    viewer->SetModelVisibility(0, true);
    viewer->SelectOnly(0);
    press("Delete");
    REQUIRE(settle());
    CHECK(titles == std::vector<wxString>{"Delete model"});
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl (2)"});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: a removal during the delete dialog doesn't change what is deleted", "[integration][ui][main_window][selection]")
{
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    window->SetConfirmDelete([this](const wxString&, const wxString&) {
        viewer->RemoveModel(0); // an earlier removal lands while the dialog is up
        return true;
    });
    window->DeleteModels({1}); // "cube.stl (2)"
    REQUIRE(settle());
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl (3)"});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: picking a hidden model in the dropdown selects it", "[integration][ui][main_window][selection]")
{
    viewer->SetModelVisibility(1, false);
    viewer->SelectOnly(0);
    pick(2); // the hidden model
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{1});
    CHECK(toolbar->GetTargetIndex() == 1);
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{true, false});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: Command/Ctrl+D duplicates the selection, asking how many times", "[integration][ui][main_window][selection]")
{
    std::vector<std::size_t> askedFor; // how many models each question was about
    std::optional<int>       copies = 2;
    window->SetAskCopies([&](std::size_t models) {
        askedFor.push_back(models);
        return copies;
    });
    const auto pressD = [this] {
        wxKeyEvent key(wxEVT_CHAR_HOOK);
        key.m_keyCode = 'D';
        key.SetControlDown(true);
        key.SetEventObject(window.get());
        window->ProcessWindowEvent(key);
    };
    REQUIRE(platform::settings::SettingsRegistry::getInstance().get("keys.duplicate") == "Ctrl+D");

    pressD(); // nothing selected: not asked
    CHECK(askedFor.empty());

    viewer->SelectOnly(0);
    viewer->ToggleSelected(1);
    copies = std::nullopt; // cancelled
    pressD();
    CHECK(askedFor == std::vector<std::size_t>{2});
    CHECK(viewer->GetModelNames().size() == 2);

    copies = 2;
    pressD();
    CHECK(viewer->GetModelNames().size() == 6);
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{2, 3, 4, 5}); // the copies
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: Show all shows every model", "[integration][ui][main_window]")
{
    auto* models = findDescendant<ui::panels::ModelsPanel>(window.get());
    REQUIRE(models != nullptr);
    wxButton* showAll = nullptr;
    for (wxWindow* child : models->GetChildren())
        if (auto* button = dynamic_cast<wxButton*>(child); button != nullptr && button->GetLabel() == "Show all")
            showAll = button;
    REQUIRE(showAll != nullptr);

    viewer->SetModelVisibility(0, false);
    viewer->SetModelVisibility(1, false);
    wxCommandEvent event(wxEVT_BUTTON, showAll->GetId());
    event.SetEventObject(showAll);
    showAll->ProcessWindowEvent(event);
    CHECK(viewer->GetModelVisibility() == std::vector<bool>{true, true});
}

TEST_CASE_METHOD(SelectionWindowFixture, "MainWindow: the Duplicate button copies the selection", "[integration][ui][main_window][selection]")
{
    window->SetAskCopies([](std::size_t) { return std::optional<int>{1}; });
    auto* models = findDescendant<ui::panels::ModelsPanel>(window.get());
    REQUIRE(models != nullptr);
    wxButton* duplicate = nullptr;
    for (wxWindow* child : models->GetChildren())
        if (auto* button = dynamic_cast<wxButton*>(child); button != nullptr && button->GetLabel() == "Duplicate")
            duplicate = button;
    REQUIRE(duplicate != nullptr);

    viewer->SelectOnly(1);
    wxCommandEvent event(wxEVT_BUTTON, duplicate->GetId());
    event.SetEventObject(duplicate);
    duplicate->ProcessWindowEvent(event);
    CHECK(viewer->GetModelNames() == std::vector<wxString>{"cube.stl", "cube.stl (2)", "cube.stl (3)"});
    CHECK(viewer->GetSelectedIndices() == std::vector<int>{2});
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

namespace
{
    // A language change, with the reload question answered by `reload`;
    // records the pending task counts it was asked with.
    class LanguageWindowFixture : public MainWindowFixture
    {
      public:
        LanguageWindowFixture()
        {
            window->SetConfirmReload([this](std::size_t pending) {
                asked.push_back(pending);
                return reload;
            });
        }

        // Sets the language, then lets the question come (it waits for set() to return).
        void changeLanguage(const std::string& language)
        {
            platform::settings::SettingsRegistry::getInstance().set(platform::locale::kLanguageSetting, language);
            tests::support::pumpUntil([this] { return !asked.empty(); });
        }

        bool                     reload = true;
        std::vector<std::size_t> asked;
    };
} // namespace

TEST_CASE("MainWindow: reloading for a new language keeps the models", "[integration][ui][main_window][locale]")
{
    // Not a ScopedFrame: the window destroys itself when it reloads.
    const RegisteredFactories factories;
    auto*                     window = new ui::windows::MainWindow();
    window->Show();
    REQUIRE(tests::support::pumpUntil([&] { return window->IsShownOnScreen(); }));
    std::vector<std::size_t> asked;
    window->SetConfirmReload([&](std::size_t pending) {
        asked.push_back(pending);
        return true;
    });
    auto* viewer = findDescendant<ModelViewerPanel>(window);
    REQUIRE(viewer != nullptr);
    const auto settle = [](ModelViewerPanel* panel) {
        return tests::support::pumpUntil([&] { return panel->tasks().idle(); });
    };
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle(viewer));
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle(viewer));
    viewer->setTransform(0, 5.0, 0.0, 0.0, 0.0, 0.0, 90.0);
    REQUIRE(settle(viewer));
    viewer->SetModelVisibility(1, false);
    viewer->SelectOnly(0);
    // Orbit and zoom, away from where framing the models would put the camera.
    auto* canvas = findDescendant<wxGLCanvas>(viewer);
    REQUIRE(canvas != nullptr);
    const auto sendMouse = [canvas](wxEventType type, int x, int y, int wheelRotation = 0) {
        wxMouseEvent event(type);
        event.SetEventObject(canvas);
        event.SetPosition(wxPoint(x, y));
        event.m_x             = x;
        event.m_y             = y;
        event.m_leftDown      = type == wxEVT_MOTION;
        event.m_wheelRotation = wheelRotation;
        event.m_wheelDelta    = 120;
        canvas->HandleWindowEvent(event);
    };
    sendMouse(wxEVT_LEFT_DOWN, 100, 100);
    sendMouse(wxEVT_MOTION, 140, 120);
    sendMouse(wxEVT_MOTION, 180, 140);
    sendMouse(wxEVT_LEFT_UP, 180, 140);
    sendMouse(wxEVT_MOUSEWHEEL, 100, 100, 240);
    const auto ids   = viewer->GetModelIds();
    const auto names = viewer->GetModelNames();
    const auto eye   = viewer->CameraEye();
    // Rotate open, and the settings page, where the language is picked.
    auto* toolbar = findDescendant<TransformToolbar>(window);
    auto* dock    = findDescendant<ui::panels::SideDock>(window);
    REQUIRE(toolbar != nullptr);
    REQUIRE(dock != nullptr);
    toolbar->OpenTool(TransformToolbar::ActiveTool::Rotate);
    REQUIRE(toolbar->GetActiveTool() == TransformToolbar::ActiveTool::Rotate);
    dock->Open(findDescendant<ui::panels::SettingsPanel>(window));
    tests::support::pumpEvents();
    // Measured with the dock open: it narrows the viewport.
    const auto corner = viewer->ProjectToCanvas(gp_Pnt(10.0, 10.0, 10.0));

    platform::settings::SettingsRegistry::getInstance().set(platform::locale::kLanguageSetting, "fr");
    ui::windows::MainWindow* reloaded = nullptr;
    REQUIRE(tests::support::pumpUntil([&] {
        reloaded = dynamic_cast<ui::windows::MainWindow*>(wxTheApp->GetTopWindow());
        return reloaded != nullptr && reloaded != window;
    }));
    CHECK(asked == std::vector<std::size_t>{0});
    CHECK(platform::locale::LocaleManager::get().getLanguage() == "fr");
    CHECK(reloaded->GetMenuBar()->FindMenu("Fichier") != wxNOT_FOUND);

    auto* moved = findDescendant<ModelViewerPanel>(reloaded);
    REQUIRE(moved != nullptr);
    REQUIRE(tests::support::pumpUntil([&] { return moved->GetModelIds().size() == 2; }));
    CHECK(moved->GetModelIds() == ids);
    CHECK(moved->GetModelNames() == names);
    CHECK(moved->GetModelVisibility() == std::vector<bool>{true, false});
    CHECK(moved->GetTransform(0).moveX == 5.0);
    CHECK(moved->GetTransform(0).rotateZ == 90.0);
    CHECK(moved->GetSelectedIndices() == std::vector<int>{0});
    CHECK(moved->IsHighlighted(0));
    for (std::size_t axis = 0; axis < 3; ++axis)
        CHECK(std::abs(moved->CameraEye()[axis] - eye[axis]) < 1e-6);
    const auto movedCorner = moved->ProjectToCanvas(gp_Pnt(10.0, 10.0, 10.0));
    CHECK(std::abs(movedCorner.x - corner.x) <= 1);
    CHECK(std::abs(movedCorner.y - corner.y) <= 1);
    auto* movedToolbar = findDescendant<TransformToolbar>(reloaded);
    auto* movedDock    = findDescendant<ui::panels::SideDock>(reloaded);
    REQUIRE(movedToolbar != nullptr);
    REQUIRE(movedDock != nullptr);
    CHECK(movedToolbar->GetActiveTool() == TransformToolbar::ActiveTool::Rotate);
    CHECK(dynamic_cast<ui::panels::SettingsPanel*>(movedDock->CurrentPage()) != nullptr);
    CHECK(movedDock->IsOpenedManually());
    // New models never take a carried-over id.
    REQUIRE(moved->loadFile(kCube));
    REQUIRE(settle(moved));
    REQUIRE(moved->GetModelIds().size() == 3);
    CHECK(moved->GetModelIds()[2] > std::max(ids[0], ids[1]));

    reloaded->Destroy();
    tests::support::pumpEvents();
}

TEST_CASE_METHOD(LanguageWindowFixture, "MainWindow: \"Later\" keeps the window and its language", "[integration][ui][main_window][locale]")
{
    reload = false;
    changeLanguage("de");

    CHECK(asked == std::vector<std::size_t>{0});
    CHECK(wxTheApp->GetTopWindow() == window.get());
    CHECK(platform::locale::LocaleManager::get().getLanguage() == "en");
}

TEST_CASE_METHOD(LanguageWindowFixture, "MainWindow: no reload while tasks are pending", "[integration][ui][main_window][locale]")
{
    REQUIRE(viewer->loadFile(kCube));
    REQUIRE(settle());
    hold();
    changeLanguage("es");

    CHECK(asked == std::vector<std::size_t>{1});
    CHECK(platform::locale::LocaleManager::get().getLanguage() == "en");
    viewer->tasks().cancelAllAndWait();
}

TEST_CASE_METHOD(LanguageWindowFixture, "MainWindow: no question when the language shown doesn't change", "[integration][ui][main_window][locale]")
{
    changeLanguage("en");
    tests::support::pumpEvents();

    CHECK(asked.empty());
}
