#include "two_column_layout.hpp"

namespace ui::layouts
{
    TwoColumnLayout::TwoColumnLayout(wxWindow* parent)
        : BaseLayout(parent)
    {
        rootSizer_ = new wxBoxSizer(wxHORIZONTAL);
        parent_->SetSizer(rootSizer_);
    }

    void TwoColumnLayout::AddLeft(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        rootSizer_->Add(widget, 1, wxEXPAND);
    }

    void TwoColumnLayout::AddRight(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        rootSizer_->Add(widget, 1, wxEXPAND);
    }
} // namespace ui::layouts
