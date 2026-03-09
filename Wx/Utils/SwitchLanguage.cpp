#include "SwitchLanguage.hpp"

#include "../Wx.hpp"

namespace wx::utils
{

    void switchLanguage(wxFrame*&                     frame,
                        const std::string&            code,
                        std::function<wxFrame*()>     createFrame,
                        std::function<void(wxFrame*)> finalize,
                        const wx::config::AppConfig&  cfg)
    {
        if (!frame)
            return;

        // Save geometry
        const auto pos  = frame->GetPosition();
        const auto size = frame->GetSize();

        // Destroy current frame
        frame->Destroy();
        frame = nullptr;

        // Re-init locale
        wx::locale::Locale::init(code, "splinter3D");

        // Create new frame
        wxFrame* nf = createFrame();
        if (!nf)
            return;

        // Restore geometry
        nf->SetPosition(pos);
        nf->SetSize(size);

        // Reapply theme and font size using global Theme
        auto& theme = wx::theme::Theme::get();
        if (cfg.dark)
            theme.apply(nf);
        if (cfg.fontSize != 11)
            theme.applyFontSize(nf, cfg.fontSize);

        // Call finalizer to let caller do class-specific restore (menus, show)
        if (finalize)
            finalize(nf);

        // Assign back
        frame = nf;
    }

} // namespace wx::utils
