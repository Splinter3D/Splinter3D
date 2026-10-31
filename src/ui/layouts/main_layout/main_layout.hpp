#pragma once

#include "ui/layouts/base_layout/base_layout.hpp"

#include <wx/sizer.h>
#include <wx/splitter.h>

namespace ui::layouts
{
    class MainLayout : public BaseLayout
    {
      public:
        explicit MainLayout(wxWindow* parent);

        void AddTop(wxWindow* widget) override;
        void AddLeft(wxWindow* widget) override;
        void AddCenter(wxWindow* widget) override;
        void AddRight(wxWindow* widget) override;
        void AddBottom(wxWindow* widget) override;

      private:
        wxBoxSizer* rootSizer_ = nullptr;

        // horizontal band: left | center | right
        wxBoxSizer* middleSizer_ = nullptr;
    };
} // namespace ui::layouts
