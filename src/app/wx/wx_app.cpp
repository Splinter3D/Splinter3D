#include "wx_app.hpp"

#include "app/app.hpp"
#include "app/bootstrap/bootstrap.hpp"
#include "core/utils/data_root.hpp"
#include "core/utils/os_compatibility.hpp"
#include "platform/benchmark/benchmark.hpp"
#include "platform/threading/dispatcher.hpp"

#include <filesystem>
#include <vector>
#include <wx/image.h>
#include <wx/stdpaths.h>

namespace app
{
    namespace
    {
        // Wide strings so non-ASCII install paths survive on Windows.
        std::filesystem::path findWordmark()
        {
            const std::filesystem::path executable(
                wxStandardPaths::Get().GetExecutablePath().ToStdWstring());
            return core::utils::findAssetFile(executable, "icons/logo_full_no_bg.png");
        }
    } // namespace

    namespace
    {
        constexpr int kSignalPollMs = 100;
        constexpr int kFrameTickMs  = 16; // ~60 Hz
    } // namespace

    // wx creates the app before Initialize() starts GTK, so this is early enough.
    WxApp::WxApp()
    {
        core::utils::preferX11Backend();
    }

    WxApp::~WxApp() = default;

    bool WxApp::OnInit()
    {
        if (!wxApp::OnInit())
            return false;

        wxInitAllImageHandlers();

        const auto wordmarkPath = findWordmark();
        if (!wordmarkPath.empty())
        {
            wxImage wordmark(wxString(wordmarkPath.wstring()), wxBITMAP_TYPE_PNG);
            if (wordmark.IsOk())
            {
                wordmark.Rescale(520, 150, wxIMAGE_QUALITY_HIGH);
                splash_screen_ = new wxSplashScreen(
                    wxBitmap(wordmark),
                    wxSPLASH_CENTRE_ON_SCREEN | wxSPLASH_NO_TIMEOUT,
                    0,
                    nullptr,
                    wxID_ANY,
                    wxDefaultPosition,
                    wxDefaultSize,
                    wxSIMPLE_BORDER | wxSTAY_ON_TOP);
                splash_screen_->Show();
                wxYield();
            }
        }

        Bootstrap bootstrap;
        application_ = bootstrap.build(this->argc, this->argv);

        if (!application_)
            return false;

        const bool initialized = application_->init();
        if (splash_screen_ != nullptr)
        {
            splash_screen_->Destroy();
            splash_screen_ = nullptr;
        }

        // Signal handlers can't safely touch wx, so they only set a flag
        // (core::utils::InstallSignalHandlers(), called by Bootstrap); poll it
        // from the GUI thread.
        signal_timer_.Bind(wxEVT_TIMER, &WxApp::onSignalPoll, this);
        signal_timer_.Start(kSignalPollMs);

        frame_timer_.Bind(wxEVT_TIMER, &WxApp::onFrameTick, this);
        frame_timer_.Start(kFrameTickMs);
        return initialized;
    }

    void WxApp::onFrameTick(wxTimerEvent&)
    {
        platform::benchmark::FrameClock::tick();
    }

    void WxApp::onSignalPoll(wxTimerEvent&)
    {
        if (!core::utils::SignalReceived())
            return;
        core::utils::ClearSignal();
        signal_timer_.Stop();

        // Close every window as if the user did (copy: closing edits the list),
        // then leave the main loop so OnExit() runs and the process exits normally.
        const std::vector<wxWindow*> windows(wxTopLevelWindows.begin(), wxTopLevelWindows.end());
        for (wxWindow* window : windows)
            window->Close(true);
        ExitMainLoop();
    }

    int WxApp::OnExit()
    {
        if (application_)
        {
            application_->shutdown();
            application_.reset();
        }
        platform::threading::Dispatcher::getInstance().save();

        return wxApp::OnExit();
    }
} // namespace app
