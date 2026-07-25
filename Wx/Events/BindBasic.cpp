#include "BindBasic.hpp"

#include "../Ids.hpp"
#include "../Wx.hpp"

namespace wx::events
{

    void bindBasic(wxFrame* m_frame)
    {
        if (!m_frame)
            return;

        // File
        m_frame->Bind(wxEVT_MENU, [m_frame](wxCommandEvent&) { wx::widgets::info(_("New file created (demo)."), _("New")); }, wxID_NEW);
        m_frame->Bind(wxEVT_MENU, [m_frame](wxCommandEvent&) {
        if (auto p = wx::widgets::openFile(m_frame))
            wx::widgets::info(_("Opened: ") + *p, _("Open")); }, wxID_OPEN);
        m_frame->Bind(wxEVT_MENU, [m_frame](wxCommandEvent&) { m_frame->Close(); }, wxID_EXIT);
        m_frame->Bind(wxEVT_MENU, [](wxCommandEvent&) { wx::widgets::info(_("wxWidgets Demo\nAll major widgets in one place."), _("About")); }, wxID_ABOUT);

        // Fullscreen
        m_frame->Bind(wxEVT_CHAR_HOOK, [m_frame](wxKeyEvent& e) {
            if (e.GetKeyCode() == WXK_F11)
            {
                bool go = !m_frame->IsFullScreen();
                m_frame->ShowFullScreen(go, wxFULLSCREEN_ALL);
                if (auto* bar = m_frame->GetMenuBar())
                    bar->Check(IDs::BASIC::FULLSCREEN, go);
            }
            else
                e.Skip();
        });
        m_frame->Bind(wxEVT_MENU, [m_frame](wxCommandEvent& e) { m_frame->ShowFullScreen(e.IsChecked(), wxFULLSCREEN_ALL); }, IDs::BASIC::FULLSCREEN);

        // Theme toggle
        m_frame->Bind(wxEVT_MENU, [m_frame](wxCommandEvent& e) {
        auto& theme = wx::theme::Theme::get();
        theme.setDark(e.IsChecked());
        theme.apply(m_frame);
        m_frame->SetStatusText(theme.isDark() ? _("Dark mode ON") : _("Light mode ON"));
        auto cfg = wx::config::AppConfig::load(); cfg.dark = theme.isDark(); cfg.save(); }, IDs::BASIC::THEME_TOGGLE);
    }

} // namespace wx::events
