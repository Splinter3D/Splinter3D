#include "BindEvents.hpp"

#include "BindBasic.hpp"
#include "BindFont.hpp"
#include "BindLang.hpp"

namespace wx::events
{

    BindEvents::BindEvents(wxFrame* frame) : m_frame(frame)
    { }

    void BindEvents::bindAll(const std::function<void(const std::string&)>& onSwitch)
    {
        if (!m_frame)
            return;

        bindBasic(m_frame);
        bindFont(m_frame);
        bindLang(m_frame, onSwitch);
    }

} // namespace wx::events
