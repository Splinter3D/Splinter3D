#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <cstdio>
#include <stdexcept>
#include <wx/app.h>
#include <wx/image.h>
#include <wx/init.h>
#include <wx/log.h>

namespace
{
    // Starts wxWidgets once for the whole integration run (windows, events, GL),
    // without entering its main loop: tests drive events synchronously and pump
    // pending ones with tests::support::pumpEvents().
    //
    // Runs from testRunStarting, not at startup, so listing tests (what CTest does
    // to discover them) never needs a display.
    class WxEnvironmentListener : public Catch::EventListenerBase
    {
      public:
        using Catch::EventListenerBase::EventListenerBase;

        void testRunStarting(const Catch::TestRunInfo&) override
        {
            static char  programName[] = "splinter3D_integration_tests";
            static char* argv[]        = {programName, nullptr};
            int          argc          = 1;

            wxApp::SetInstance(new wxApp());
            if (!wxEntryStart(argc, argv) || !wxTheApp->CallOnInit())
                throw std::runtime_error("Failed to initialize wxWidgets (is a display available? "
                                         "On headless Linux, run under xvfb-run).");

            wxInitAllImageHandlers();
            // The default GUI log target shows a modal dialog for wxLogError /
            // wxLogWarning, which would block the run forever.
            delete wxLog::SetActiveTarget(new wxLogStderr());
        }

        void testRunEnded(const Catch::TestRunStats&) override
        {
            if (wxTheApp != nullptr)
                wxTheApp->OnExit();
            wxEntryCleanup();
        }
    };
} // namespace

CATCH_REGISTER_LISTENER(WxEnvironmentListener)
