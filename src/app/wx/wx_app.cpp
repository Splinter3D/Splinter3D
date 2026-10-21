#include "wx_app.hpp"

#include "app/app.hpp"
#include "app/bootstrap/bootstrap.hpp"
#include "core/utils/os_compatibility.hpp"

#include <filesystem>
#include <vector>
#include <wx/image.h>
#include <wx/stdpaths.h>

namespace app
{
    namespace
    {
        std::filesystem::path findWordmark()
        {
            const auto executable = std::filesystem::path(
                wxStandardPaths::Get().GetExecutablePath().ToStdString());
            const std::vector<std::filesystem::path> roots{
                std::filesystem::current_path(), executable.parent_path(),
                executable.parent_path().parent_path(),
                executable.parent_path().parent_path().parent_path(),
                executable.parent_path().parent_path().parent_path().parent_path()};

            for (const auto& root : roots)
            {
                const auto candidate = root / "assets" / "icons" / "logo_full_no_bg.png";
                if (std::filesystem::exists(candidate))
                    return candidate;
            }
            return {};
        }
    } // namespace

    namespace
    {
        constexpr int kSignalPollMs = 100;
    } // namespace

    WxApp::WxApp()  = default;
    WxApp::~WxApp() = default;

    bool WxApp::OnInit()
    {
        if (!wxApp::OnInit())
            return false;

        wxInitAllImageHandlers();

        const auto wordmarkPath = findWordmark();
        if (!wordmarkPath.empty())
        {
            wxImage wordmark(wordmarkPath.string(), wxBITMAP_TYPE_PNG);
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
        return initialized;
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

        return wxApp::OnExit();
    }
} // namespace app
