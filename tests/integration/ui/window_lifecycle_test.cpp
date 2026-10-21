#include "support/wx_helpers.hpp"
#include "ui/framework/wx/events/event_manager.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registration.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/windows/main_window/main_window.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wx/frame.h>
#include <wx/weakref.h>

// Windows that close themselves: not wrapped in ScopedFrame, which would destroy them a
// second time.

namespace ids = ui::framework::wx::ids;
using tests::support::pumpUntil;
using tests::support::sendMenuCommand;

TEST_CASE("WindowEvents: closing a frame destroys it", "[integration][ui][events]")
{
    auto*                                   frame = new wxFrame(nullptr, wxID_ANY, "Close test");
    wxWeakRef<wxFrame>                      alive(frame);
    ui::framework::wx::events::EventManager manager(frame);
    manager.bindAll();
    frame->Show();

    frame->Close();

    REQUIRE(pumpUntil([&alive] { return !alive; }));
}

TEST_CASE("EventManager: a null frame binds nothing", "[integration][ui][events]")
{
    ui::framework::wx::events::EventManager manager(nullptr);
    manager.bindAll();
    SUCCEED();
}

TEST_CASE("MainWindow: File > Exit closes the window", "[integration][ui][main_window]")
{
    ui::menus::RegisterAllMenus();
    ui::toolbars::RegisterAllToolBars();
    ui::layouts::RegisterAllLayouts();
    ui::panels::RegisterAllPanels();

    auto*                              window = new ui::windows::MainWindow();
    wxWeakRef<ui::windows::MainWindow> alive(window);
    window->Show();

    sendMenuCommand(window, ids::file::kExit);

    REQUIRE(pumpUntil([&alive] { return !alive; }));
}
