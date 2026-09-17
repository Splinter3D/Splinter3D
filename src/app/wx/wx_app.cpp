#include "wx_app.hpp"

#include "app/app.hpp"
#include "app/bootstrap/bootstrap.hpp"

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
        return initialized;
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
