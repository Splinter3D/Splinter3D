#include "main_layout.hpp"

namespace ui::layouts
{
    MainLayout::MainLayout(wxWindow* parent)
        : BaseLayout(parent)
    {
        rootSizer_ = new wxBoxSizer(wxVERTICAL);
        parent_->SetSizer(rootSizer_);

        middleSizer_ = new wxBoxSizer(wxHORIZONTAL);
        // proportion=1 so the middle band eats remaining vertical space
        rootSizer_->Add(middleSizer_, 1, wxEXPAND);
    }

    void MainLayout::AddTop(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        rootSizer_->Insert(0, widget, 0, wxEXPAND);
    }

    void MainLayout::AddLeft(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        middleSizer_->Add(widget, 0, wxEXPAND);
    }

    void MainLayout::AddCenter(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        middleSizer_->Add(widget, 1, wxEXPAND); // takes remaining width
    }

    void MainLayout::AddRight(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        middleSizer_->Add(widget, 0, wxEXPAND);
    }

    void MainLayout::AddBottom(wxWindow* widget)
    {
        if (widget == nullptr)
            return;
        rootSizer_->Add(widget, 0, wxEXPAND); // e.g. Console
    }
} // namespace ui::layouts
