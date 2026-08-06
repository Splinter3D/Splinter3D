#pragma once

#include <wx/wx.h>

namespace ui::panels
{
    // Pure visual proof-of-life for every encapsulated widget in
    // ui/widgets/. No business logic: it just stacks one instance of each
    // so they can be seen and interacted with.
    class WidgetsShowcasePanel : public wxPanel
    {
      public:
        explicit WidgetsShowcasePanel(wxWindow* parent);
    };
} // namespace ui::panels
