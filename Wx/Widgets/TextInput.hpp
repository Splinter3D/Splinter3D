#pragma once
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  TextInput — fluent builder for wxTextCtrl
    //
    //  Usage:
    //    auto* t = wx::TextInput(parent)
    //                  .hint("Type here…")
    //                  .multiline()
    //                  .build();
    // ─────────────────────────────────────────────
    class TextInput
    {
      public:
        explicit TextInput(wxWindow* parent) : m_parent(parent)
        { }

        TextInput& hint(const wxString& h)
        {
            m_hint = h;
            return *this;
        }

        TextInput& value(const wxString& v)
        {
            m_value = v;
            return *this;
        }

        TextInput& height(int px)
        {
            m_height = px;
            return *this;
        }

        TextInput& password()
        {
            m_password = true;
            return *this;
        }

        TextInput& multiline()
        {
            m_multi = true;
            return *this;
        }

        TextInput& readOnly()
        {
            m_readOnly = true;
            return *this;
        }

        wxTextCtrl* build() const
        {
            long style = 0;
            if (m_password)
                style |= wxTE_PASSWORD;
            if (m_multi)
                style |= wxTE_MULTILINE;
            if (m_readOnly)
                style |= wxTE_READONLY;

            wxSize size = m_height > 0 ? wxSize(-1, m_height) : wxDefaultSize;
            auto*  ctrl = new wxTextCtrl(m_parent, wxID_ANY, m_value,
                                         wxDefaultPosition, size, style);
            if (!m_hint.empty())
                ctrl->SetHint(m_hint);
            return ctrl;
        }

      private:
        wxWindow* m_parent = nullptr;
        wxString  m_hint;
        wxString  m_value;
        int       m_height   = 0;
        bool      m_password = false;
        bool      m_multi    = false;
        bool      m_readOnly = false;
    };

    // ─────────────────────────────────────────────
    //  SpinInput — fluent builder for wxSpinCtrl
    //
    //  Usage:
    //    auto* s = wx::SpinInput(parent)
    //                  .range(0, 200)
    //                  .value(42)
    //                  .build();
    // ─────────────────────────────────────────────
    class SpinInput
    {
      public:
        explicit SpinInput(wxWindow* parent) : m_parent(parent)
        { }

        SpinInput& range(int lo, int hi)
        {
            m_min = lo;
            m_max = hi;
            return *this;
        }

        SpinInput& value(int v)
        {
            m_val = v;
            return *this;
        }

        wxSpinCtrl* build() const
        {
            return new wxSpinCtrl(m_parent, wxID_ANY,
                                  wxString::Format("%d", m_val),
                                  wxDefaultPosition, wxDefaultSize,
                                  wxSP_ARROW_KEYS, m_min, m_max, m_val);
        }

      private:
        wxWindow* m_parent = nullptr;
        int       m_min    = 0;
        int       m_max    = 100;
        int       m_val    = 0;
    };

} // namespace wx