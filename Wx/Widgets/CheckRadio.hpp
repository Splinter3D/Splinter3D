#pragma once
#include <functional>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  CheckBox — fluent builder for wxCheckBox
    //
    //  Usage:
    //    auto* cb = wx::CheckBox(parent)
    //                   .label("Enable feature")
    //                   .checked(true)
    //                   .onToggle([](bool on){ ... })
    //                   .build();
    // ─────────────────────────────────────────────
    class CheckBox
    {
      public:
        explicit CheckBox(wxWindow* parent) : m_parent(parent)
        { }

        CheckBox& label(const wxString& lbl)
        {
            m_label = lbl;
            return *this;
        }

        CheckBox& checked(bool v = true)
        {
            m_checked = v;
            return *this;
        }

        CheckBox& disabled()
        {
            m_enabled = false;
            return *this;
        }

        CheckBox& onToggle(std::function<void(bool)> fn)
        {
            m_onToggle = std::move(fn);
            return *this;
        }

        wxCheckBox* build() const
        {
            auto* cb = new wxCheckBox(m_parent, wxID_ANY, m_label);
            cb->SetValue(m_checked);
            if (!m_enabled)
                cb->Disable();
            if (m_onToggle)
                cb->Bind(wxEVT_CHECKBOX, [fn = m_onToggle](wxCommandEvent& e) {
                    fn(e.IsChecked());
                });
            return cb;
        }

      private:
        wxWindow*                 m_parent = nullptr;
        wxString                  m_label;
        bool                      m_checked = false;
        bool                      m_enabled = true;
        std::function<void(bool)> m_onToggle;
    };

    // ─────────────────────────────────────────────
    //  RadioBox — fluent builder for wxRadioBox
    //
    //  Usage:
    //    auto* rb = wx::RadioBox(parent)
    //                   .title("Select mode")
    //                   .choices({"A", "B", "C"})
    //                   .columns(1)
    //                   .onChange([](const wxString& sel){ ... })
    //                   .build();
    // ─────────────────────────────────────────────
    class RadioBox
    {
      public:
        explicit RadioBox(wxWindow* parent) : m_parent(parent)
        { }

        RadioBox& title(const wxString& t)
        {
            m_title = t;
            return *this;
        }

        RadioBox& columns(int c)
        {
            m_cols = c;
            return *this;
        }

        RadioBox& choices(const wxArrayString& c)
        {
            m_choices = c;
            return *this;
        }

        RadioBox& onChange(std::function<void(const wxString&)> fn)
        {
            m_onChange = std::move(fn);
            return *this;
        }

        wxRadioBox* build() const
        {
            auto* rb = new wxRadioBox(m_parent, wxID_ANY, m_title,
                                      wxDefaultPosition, wxDefaultSize,
                                      m_choices, m_cols, wxRA_SPECIFY_COLS);
            if (m_onChange)
                rb->Bind(wxEVT_RADIOBOX, [fn = m_onChange, rb](wxCommandEvent&) {
                    fn(rb->GetStringSelection());
                });
            return rb;
        }

      private:
        wxWindow*                            m_parent = nullptr;
        wxString                             m_title;
        wxArrayString                        m_choices;
        int                                  m_cols = 1;
        std::function<void(const wxString&)> m_onChange;
    };

} // namespace wx