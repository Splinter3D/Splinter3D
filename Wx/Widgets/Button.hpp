#pragma once
#include <functional>
#include <wx/tglbtn.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  Button — fluent builder for wxButton
    //
    //  Usage:
    //    auto* btn = wx::Button(parent)
    //                    .label("Click me")
    //                    .onClick([]{ wxMessageBox("Hi!"); })
    //                    .build();
    // ─────────────────────────────────────────────
    class Button
    {
      public:
        explicit Button(wxWindow* parent) : m_parent(parent)
        { }

        Button& label(const wxString& lbl)
        {
            m_label = lbl;
            return *this;
        }

        Button& tooltip(const wxString& tip)
        {
            m_tooltip = tip;
            return *this;
        }

        Button& disabled()
        {
            m_enabled = false;
            return *this;
        }

        Button& onClick(std::function<void()> fn)
        {
            m_onClick = std::move(fn);
            return *this;
        }

        wxButton* build() const
        {
            auto* btn = new wxButton(m_parent, wxID_ANY, m_label);
            if (!m_enabled)
                btn->Disable();
            if (!m_tooltip.empty())
                btn->SetToolTip(m_tooltip);
            if (m_onClick)
                btn->Bind(wxEVT_BUTTON, [fn = m_onClick](wxCommandEvent&) { fn(); });
            return btn;
        }

      private:
        wxWindow*             m_parent = nullptr;
        wxString              m_label;
        wxString              m_tooltip;
        bool                  m_enabled = true;
        std::function<void()> m_onClick;
    };

    // ─────────────────────────────────────────────
    //  ToggleButton — fluent builder for wxToggleButton
    //
    //  Usage:
    //    auto* t = wx::ToggleButton(parent)
    //                  .label("Toggle me")
    //                  .onToggle([](bool on){ ... })
    //                  .build();
    // ─────────────────────────────────────────────
    class ToggleButton
    {
      public:
        explicit ToggleButton(wxWindow* parent) : m_parent(parent)
        { }

        ToggleButton& label(const wxString& lbl)
        {
            m_label = lbl;
            return *this;
        }

        ToggleButton& onToggle(std::function<void(bool)> fn)
        {
            m_onToggle = std::move(fn);
            return *this;
        }

        wxToggleButton* build() const
        {
            auto* btn = new wxToggleButton(m_parent, wxID_ANY, m_label);
            if (m_onToggle)
                btn->Bind(wxEVT_TOGGLEBUTTON, [fn = m_onToggle](wxCommandEvent& e) {
                    fn(e.IsChecked());
                });
            return btn;
        }

      private:
        wxWindow*                 m_parent = nullptr;
        wxString                  m_label;
        std::function<void(bool)> m_onToggle;
    };

} // namespace wx