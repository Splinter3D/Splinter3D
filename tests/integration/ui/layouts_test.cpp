#include "support/wx_helpers.hpp"
#include "ui/framework/wx/ids/ids.hpp"
#include "ui/layouts/layout_registration.hpp"
#include "ui/layouts/layout_registry.hpp"
#include "ui/menus/menu_registration.hpp"
#include "ui/menus/menu_registry.hpp"
#include "ui/panels/panel_registration.hpp"
#include "ui/panels/panel_registry.hpp"
#include "ui/toolbars/toolbar_registration.hpp"
#include "ui/toolbars/toolbar_registry.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <wx/panel.h>
#include <wx/sizer.h>

namespace ids = ui::framework::wx::ids;
using tests::support::ScopedFrame;
using ui::layouts::BaseLayout;
using ui::layouts::LayoutRegistry;

namespace
{
    std::unique_ptr<BaseLayout> createLayout(int id, wxWindow* parent)
    {
        ui::layouts::RegisterAllLayouts();
        return std::unique_ptr<BaseLayout>(LayoutRegistry::getInstance().Create(id, parent));
    }

    wxPanel* panel(wxWindow* parent)
    {
        return new wxPanel(parent);
    }
} // namespace

TEST_CASE("MainLayout: top / left / center / right / bottom regions", "[integration][ui][layout]")
{
    ScopedFrame<> frame(nullptr, wxID_ANY, "MainLayout test");
    auto          layout = createLayout(ids::layouts::kMain, frame.get());
    REQUIRE(layout != nullptr);

    wxSizer* root = frame->GetSizer();
    REQUIRE(root != nullptr);
    // A horizontal middle row, between whatever goes on top and at the bottom.
    REQUIRE(root->GetItemCount() == 1);
    wxSizer* middle = root->GetItem(static_cast<size_t>(0))->GetSizer();
    REQUIRE(middle != nullptr);

    wxPanel* top    = panel(frame.get());
    wxPanel* left   = panel(frame.get());
    wxPanel* center = panel(frame.get());
    wxPanel* right  = panel(frame.get());
    wxPanel* bottom = panel(frame.get());
    layout->AddBottom(bottom);
    layout->AddTop(top);
    layout->AddLeft(left);
    layout->AddCenter(center);
    layout->AddRight(right);

    // Null widgets are ignored everywhere.
    layout->AddTop(nullptr);
    layout->AddLeft(nullptr);
    layout->AddCenter(nullptr);
    layout->AddRight(nullptr);
    layout->AddBottom(nullptr);

    REQUIRE(root->GetItemCount() == 3);
    CHECK(root->GetItem(static_cast<size_t>(0))->GetWindow() == top); // inserted first
    CHECK(root->GetItem(static_cast<size_t>(2))->GetWindow() == bottom);

    REQUIRE(middle->GetItemCount() == 3);
    CHECK(middle->GetItem(static_cast<size_t>(0))->GetWindow() == left);
    CHECK(middle->GetItem(static_cast<size_t>(1))->GetWindow() == center);
    CHECK(middle->GetItem(static_cast<size_t>(2))->GetWindow() == right);
    // Only the center stretches to fill the remaining width.
    CHECK(middle->GetItem(static_cast<size_t>(1))->GetProportion() == 1);
    CHECK(middle->GetItem(static_cast<size_t>(0))->GetProportion() == 0);
}

TEST_CASE("TwoColumnLayout: two equal columns, other regions ignored", "[integration][ui][layout]")
{
    ScopedFrame<> frame(nullptr, wxID_ANY, "TwoColumnLayout test");
    auto          layout = createLayout(ids::layouts::kTwoColumn, frame.get());
    REQUIRE(layout != nullptr);

    wxPanel* left  = panel(frame.get());
    wxPanel* right = panel(frame.get());
    layout->AddLeft(left);
    layout->AddRight(right);
    layout->AddLeft(nullptr);
    layout->AddRight(nullptr);
    // Not supported by this layout: BaseLayout's defaults ignore them.
    layout->AddTop(panel(frame.get()));
    layout->AddCenter(panel(frame.get()));
    layout->AddBottom(panel(frame.get()));

    wxSizer* root = frame->GetSizer();
    REQUIRE(root->GetItemCount() == 2);
    CHECK(root->GetItem(static_cast<size_t>(0))->GetWindow() == left);
    CHECK(root->GetItem(static_cast<size_t>(1))->GetWindow() == right);
    CHECK(root->GetItem(static_cast<size_t>(0))->GetProportion() == 1);
    CHECK(root->GetItem(static_cast<size_t>(1))->GetProportion() == 1);
}

TEST_CASE("Registries: every registered id creates its object, unknown ids give null", "[integration][ui][registry]")
{
    ui::menus::RegisterAllMenus();
    ui::toolbars::RegisterAllToolBars();
    ui::layouts::RegisterAllLayouts();
    ui::panels::RegisterAllPanels();

    ScopedFrame<> frame(nullptr, wxID_ANY, "Registries test");
    constexpr int kUnknown = ids::app::kBase + 9999;

    auto& menus = ui::menus::MenuRegistry::getInstance();
    CHECK(menus.IsRegistered(ids::menus::kFile));
    CHECK_FALSE(menus.IsRegistered(kUnknown));
    CHECK(menus.Create(kUnknown) == nullptr);
    std::unique_ptr<wxMenu> menu(menus.Create(ids::menus::kFile));
    CHECK(menu != nullptr);

    auto& toolbars = ui::toolbars::ToolBarRegistry::getInstance();
    CHECK(toolbars.IsRegistered(ids::toolbars::kTransform));
    CHECK_FALSE(toolbars.IsRegistered(kUnknown));
    CHECK(toolbars.Create(kUnknown, frame.get()) == nullptr);
    CHECK(toolbars.Create(ids::toolbars::kTransform, frame.get()) != nullptr);

    auto& layouts = LayoutRegistry::getInstance();
    CHECK(layouts.IsRegistered(ids::layouts::kMain));
    CHECK(layouts.IsRegistered(ids::layouts::kTwoColumn));
    CHECK(layouts.Create(kUnknown, frame.get()) == nullptr);

    auto& panels = ui::panels::PanelRegistry::getInstance();
    CHECK(panels.IsRegistered(ids::panels::kModelViewer));
    CHECK_FALSE(panels.IsRegistered(kUnknown));
    CHECK(panels.Create(kUnknown, frame.get()) == nullptr);
}
