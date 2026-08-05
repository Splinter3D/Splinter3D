#include "BindFont.hpp"

#include "../Ids.hpp"
#include "../Wx.hpp"

namespace wx::events
{

    void bindFont(wxFrame* m_frame)
    {
        if (!m_frame)
            return;

        auto bindFontInner = [m_frame](int id, int pts) {
            m_frame->Bind(wxEVT_MENU, [m_frame, pts](wxCommandEvent&) {
            wx::theme::Theme::get().applyFontSize(m_frame, pts);
            m_frame->SetStatusText(wxString::Format(_("Font size: %dpt"), pts));
            auto cfg = wx::config::AppConfig::load(); cfg.fontSize = pts; cfg.save(); }, id);
        };

        bindFontInner(IDs::FONT::SMALL, 9);
        bindFontInner(IDs::FONT::NORMAL, 11);
        bindFontInner(IDs::FONT::LARGE, 14);
        bindFontInner(IDs::FONT::XLARGE, 18);
    }

} // namespace wx::events
