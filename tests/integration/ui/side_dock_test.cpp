#include "support/wx_helpers.hpp"
#include "ui/panels/side_dock/side_dock.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wx/sizer.h>
#include <wx/tglbtn.h>

using tests::support::pumpEvents;
using tests::support::ScopedFrame;
using ui::panels::SideDock;

namespace
{
    // What clicking the button does: wx flips the value, then sends the event.
    void click(wxToggleButton* button)
    {
        button->SetValue(!button->GetValue());
        wxCommandEvent event(wxEVT_TOGGLEBUTTON, button->GetId());
        event.SetEventObject(button);
        button->ProcessWindowEvent(event);
        pumpEvents(); // the dock acts on the click after the event returns
    }

    struct DockFixture
    {
        ScopedFrame<> frame{nullptr, wxID_ANY, "SideDock test", wxDefaultPosition, wxSize(640, 480)};
        SideDock*     dock  = nullptr;
        wxPanel*      first = nullptr;
        wxPanel*      other = nullptr;

        DockFixture()
        {
            dock  = new SideDock(frame.get());
            first = new wxPanel(dock);
            other = new wxPanel(dock);
            dock->AddPage(first, wxBitmap(), "First");
            dock->AddPage(other, wxBitmap(), "Other");
            auto* sizer = new wxBoxSizer(wxHORIZONTAL);
            sizer->Add(new wxPanel(frame.get()), 1, wxEXPAND);
            sizer->Add(dock, 0, wxEXPAND);
            frame->SetSizer(sizer);
            frame->Show();
            pumpEvents();
        }
    };
} // namespace

TEST_CASE_METHOD(DockFixture, "SideDock: pages start closed, buttons always show", "[integration][ui][side_dock]")
{
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(first->IsShown());
    CHECK_FALSE(other->IsShown());
    REQUIRE(dock->GetButton(first) != nullptr);
    CHECK(dock->GetButton(first)->IsShown());
    CHECK(dock->GetButton(other)->IsShown());
}

TEST_CASE_METHOD(DockFixture, "SideDock: a button opens its page, a second click closes it", "[integration][ui][side_dock]")
{
    click(dock->GetButton(first));
    CHECK(dock->CurrentPage() == first);
    CHECK(first->IsShown());
    CHECK(dock->GetButton(first)->GetValue());
    CHECK(dock->IsOpenedManually());

    click(dock->GetButton(other));
    CHECK(dock->CurrentPage() == other);
    CHECK_FALSE(first->IsShown());
    CHECK_FALSE(dock->GetButton(first)->GetValue());

    click(dock->GetButton(other));
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(other->IsShown());
    CHECK_FALSE(dock->GetButton(other)->GetValue());
}

TEST_CASE_METHOD(DockFixture, "SideDock: a requested page closes on release", "[integration][ui][side_dock]")
{
    dock->RequestOpen(first);
    CHECK(dock->CurrentPage() == first);
    CHECK(dock->GetButton(first)->GetValue());
    CHECK_FALSE(dock->IsOpenedManually());

    dock->ReleaseOpen(first);
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(first->IsShown());
}

TEST_CASE_METHOD(DockFixture, "SideDock: a page opened by hand ignores release", "[integration][ui][side_dock]")
{
    click(dock->GetButton(first));
    dock->RequestOpen(first);
    dock->ReleaseOpen(first);
    CHECK(dock->CurrentPage() == first);
    CHECK(first->IsShown());
}

TEST_CASE_METHOD(DockFixture, "SideDock: a request doesn't replace the open page", "[integration][ui][side_dock]")
{
    click(dock->GetButton(other));
    dock->RequestOpen(first);
    CHECK(dock->CurrentPage() == other);
    dock->ReleaseOpen(first);
    CHECK(dock->CurrentPage() == other);
}

TEST_CASE_METHOD(DockFixture, "SideDock: clicking a requested page's button closes it", "[integration][ui][side_dock]")
{
    dock->RequestOpen(first);
    click(dock->GetButton(first));
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(dock->GetButton(first)->GetValue());
}

TEST_CASE_METHOD(DockFixture, "SideDock: once closed by hand, requests no longer open it", "[integration][ui][side_dock]")
{
    SECTION("closed while requested")
    {
        dock->RequestOpen(first);
        click(dock->GetButton(first));
    }
    SECTION("opened then closed by hand")
    {
        click(dock->GetButton(other));
        click(dock->GetButton(other));
    }
    REQUIRE(dock->CurrentPage() == nullptr);

    dock->RequestOpen(first);
    CHECK(dock->CurrentPage() == nullptr);
    CHECK_FALSE(first->IsShown());

    click(dock->GetButton(first)); // the user can still open it
    CHECK(dock->CurrentPage() == first);
}
