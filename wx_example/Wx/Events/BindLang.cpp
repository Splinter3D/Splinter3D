#include "BindLang.hpp"

#include "../Ids.hpp"
#include "../Wx.hpp"

namespace wx::events
{

    void bindLang(wxFrame* /*m_frame*/, std::function<void(const std::string&)> onSwitch)
    {
        auto bindLangInner = [onSwitch](int id, const std::string& code) {
            wxTheApp->Bind(wxEVT_MENU, [code, onSwitch](wxCommandEvent&) {
            auto cfg = wx::config::AppConfig::load(); cfg.lang = code; cfg.save();
            if (onSwitch)
                wxTheApp->CallAfter([code, onSwitch]() { onSwitch(code); }); }, id);
        };

        bindLangInner(IDs::LANG::DEFAULT, "default");
        bindLangInner(IDs::LANG::EN, "en");
        bindLangInner(IDs::LANG::FR, "fr");
        bindLangInner(IDs::LANG::ES, "es");
        bindLangInner(IDs::LANG::DE, "de");
    }

} // namespace wx::events
