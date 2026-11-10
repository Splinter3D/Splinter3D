#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"

#include <wx/sizer.h>

namespace ui::layouts
{
    // Minimal layout: two equal columns, nothing else. Demonstrates that a
    // layout only needs to implement the slots it actually uses - AddTop(),
    // AddCenter(), AddBottom() stay the BaseLayout no-op here on purpose.
    class TwoColumnLayout : public BaseLayout
    {
      public:
        explicit TwoColumnLayout(wxWindow* parent);

        void AddLeft(wxWindow* widget) override;
        void AddRight(wxWindow* widget) override;

      private:
        wxBoxSizer* rootSizer_ = nullptr;
    };
} // namespace ui::layouts
