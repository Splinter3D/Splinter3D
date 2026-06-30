#pragma once
#include <functional>
#include <wx/combobox.h>
#include <wx/listbox.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  ComboBox — fluent builder for wxComboBox (read-only dropdown)
    //
    //  Usage:
    //    auto* cb = wx::ComboBox(parent)
    //                   .placeholder("Pick a fruit")
    //                   .choices(fruits)
    //                   .onSelect([](const wxString& s){ ... })
    //                   .build();
    // ─────────────────────────────────────────────
    class ComboBox
    {
      public:
        explicit ComboBox(wxWindow* parent) : m_parent(parent)
        { }

        ComboBox& placeholder(const wxString& p)
        {
            m_placeholder = p;
            return *this;
        }

        ComboBox& choices(const wxArrayString& c)
        {
            m_choices = c;
            return *this;
        }

        ComboBox& onSelect(std::function<void(const wxString&)> fn)
        {
            m_onSelect = std::move(fn);
            return *this;
        }

        wxComboBox* build() const
        {
            auto* cb = new wxComboBox(m_parent, wxID_ANY, m_placeholder,
                                      wxDefaultPosition, wxDefaultSize,
                                      m_choices, wxCB_READONLY);
            if (m_onSelect)
                cb->Bind(wxEVT_COMBOBOX, [fn = m_onSelect, cb](wxCommandEvent&) {
                    fn(cb->GetStringSelection());
                });
            return cb;
        }

      private:
        wxWindow*                            m_parent;
        wxString                             m_placeholder;
        wxArrayString                        m_choices;
        std::function<void(const wxString&)> m_onSelect;
    };

    // ─────────────────────────────────────────────
    //  ListBox — fluent builder for wxListBox
    //
    //  Usage:
    //    auto* lb = wx::ListBox(parent)
    //                   .choices(langs)
    //                   .multiSelect()
    //                   .height(120)
    //                   .build();
    // ─────────────────────────────────────────────
    class ListBox
    {
      public:
        explicit ListBox(wxWindow* parent) : m_parent(parent)
        { }

        ListBox& choices(const wxArrayString& c)
        {
            m_choices = c;
            return *this;
        }

        ListBox& multiSelect()
        {
            m_multi = true;
            return *this;
        }

        ListBox& height(int px)
        {
            m_height = px;
            return *this;
        }

        wxListBox* build() const
        {
            long   style = m_multi ? wxLB_MULTIPLE : wxLB_SINGLE;
            wxSize size  = m_height > 0 ? wxSize(-1, m_height) : wxDefaultSize;
            return new wxListBox(m_parent, wxID_ANY,
                                 wxDefaultPosition, size,
                                 m_choices, style);
        }

      private:
        wxWindow*     m_parent = nullptr;
        wxArrayString m_choices;
        bool          m_multi  = false;
        int           m_height = 0;
    };

} // namespace wx