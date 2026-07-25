#pragma once
#include <functional>
#include <wx/gauge.h>
#include <wx/slider.h>
#include <wx/wx.h>

namespace wx::widgets
{

    // ─────────────────────────────────────────────
    //  Slider — fluent builder for wxSlider
    //
    //  Usage:
    //    auto* s = wx::Slider(parent)
    //                  .range(0, 100).value(50)
    //                  .vertical().withLabels()
    //                  .onChange([](int v){ ... })
    //                  .build();
    // ─────────────────────────────────────────────
    class Slider
    {
      public:
        explicit Slider(wxWindow* parent) : m_parent(parent)
        { }

        Slider& range(int lo, int hi)
        {
            m_min = lo;
            m_max = hi;
            return *this;
        }

        Slider& value(int v)
        {
            m_val = v;
            return *this;
        }

        Slider& vertical()
        {
            m_vert = true;
            return *this;
        }

        Slider& withLabels()
        {
            m_labels = true;
            return *this;
        }

        Slider& size(int w, int h)
        {
            m_w = w;
            m_h = h;
            return *this;
        }

        Slider& onChange(std::function<void(int)> fn)
        {
            m_onChange = std::move(fn);
            return *this;
        }

        wxSlider* build() const
        {
            long style = m_vert ? wxSL_VERTICAL : wxSL_HORIZONTAL;
            if (m_labels)
            {
                style |= wxSL_LABELS;
                if (m_vert)
                    style |= wxSL_INVERSE; // top = high, bottom = low
            }
            wxSize sz = (m_w > 0 || m_h > 0) ? wxSize(m_w, m_h) : wxDefaultSize;
            auto*  sl = new wxSlider(m_parent, wxID_ANY, m_val, m_min, m_max,
                                     wxDefaultPosition, sz, style);
            if (m_onChange)
                sl->Bind(wxEVT_SLIDER, [fn = m_onChange, sl](wxCommandEvent&) {
                    fn(sl->GetValue());
                });
            return sl;
        }

      private:
        wxWindow*                m_parent = nullptr;
        int                      m_min    = 0;
        int                      m_max    = 100;
        int                      m_val    = 0;
        int                      m_w      = -1;
        int                      m_h      = -1;
        bool                     m_vert   = false;
        bool                     m_labels = false;
        std::function<void(int)> m_onChange;
    };

    // ─────────────────────────────────────────────
    //  Gauge — fluent builder for wxGauge
    //
    //  Usage:
    //    auto* g = wx::Gauge(parent).range(100).value(50).build();
    // ─────────────────────────────────────────────
    class Gauge
    {
      public:
        explicit Gauge(wxWindow* parent) : m_parent(parent)
        { }

        Gauge& range(int r)
        {
            m_range = r;
            return *this;
        }

        Gauge& value(int v)
        {
            m_val = v;
            return *this;
        }

        wxGauge* build() const
        {
            auto* g = new wxGauge(m_parent, wxID_ANY, m_range);
            g->SetValue(m_val);
            return g;
        }

      private:
        wxWindow* m_parent = nullptr;
        int       m_range  = 100;
        int       m_val    = 0;
    };

} // namespace wx