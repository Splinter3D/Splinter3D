#pragma once
#include "../Wx/Wx.hpp"

#include <wx/notebook.h>

class DialogsPanel : public wxPanel
{
  public:
    explicit DialogsPanel(wxNotebook* parent) : wxPanel(parent)
    {
        auto* sizer  = new wxBoxSizer(wxVERTICAL);
        auto  addBtn = [&](const wxString& lbl, std::function<void()> fn) {
            sizer->Add(
                wx::widgets::Button(this).label(lbl).onClick(std::move(fn)).build(),
                0, wxALL | wxEXPAND, 8);
        };

        sizer->Add(new wxStaticText(this, wxID_ANY, _("— Dialogs & Popups showcase —")), 0, wxALL, 10);
        addBtn(_("Info message"), [] { wx::widgets::info(_("This is an info message.")); });
        addBtn(_("Warning message"), [] { wx::widgets::warning(_("This is a warning!")); });
        addBtn(_("Error message"), [] { wx::widgets::error(_("Something went wrong.")); });
        addBtn(_("Yes / No confirm"), [] {
            wx::widgets::info(
                wx::widgets::confirm(_("Do you like wxWidgets?")) ? _("Great!") : _("Give it time ;)"));
        });
        addBtn(_("Ask text input"), [this] {
            if (auto name = wx::widgets::askText(this, _("What is your name?"), _("Enter name"), "World"))
                wx::widgets::info(_("Hello, ") + *name + "!");
        });
        addBtn(_("Ask number input"), [this] {
            if (auto n = wx::widgets::askNumber(this, _("Enter a number:"), _("Pick"), 42, 0, 1000))
                wx::widgets::info(wxString::Format(_("You entered: %ld"), *n));
        });
        SetSizer(sizer);
    }
};
