#pragma once

#include <wx/window.h>

namespace ui::layouts
{
    // A Layout only answers "where does each widget go?".
    // It knows nothing about commands, business logic, or panel types.
    class BaseLayout
    {
      public:
        explicit BaseLayout(wxWindow* parent) : parent_(parent)
        { }

        virtual ~BaseLayout() = default;

        virtual void AddTop(wxWindow* widget)
        {
            (void) widget;
        }

        virtual void AddLeft(wxWindow* widget)
        {
            (void) widget;
        }

        virtual void AddCenter(wxWindow* widget)
        {
            (void) widget;
        }

        virtual void AddRight(wxWindow* widget)
        {
            (void) widget;
        }

        virtual void AddBottom(wxWindow* widget)
        {
            (void) widget;
        }

      protected:
        wxWindow* parent_ = nullptr;
    };
} // namespace ui::layouts
