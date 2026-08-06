#include "wx_app.hpp"

#include "app/app.hpp"
#include "app/bootstrap/bootstrap.hpp"

namespace app
{
    WxApp::WxApp()  = default;
    WxApp::~WxApp() = default;

    bool WxApp::OnInit()
    {
        if (!wxApp::OnInit())
            return false;

        Bootstrap bootstrap;
        application_ = bootstrap.build(this->argc, this->argv);

        if (!application_)
            return false;

        return application_->init();
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
