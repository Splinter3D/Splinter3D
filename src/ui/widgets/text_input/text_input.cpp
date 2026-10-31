#include "text_input.hpp"

namespace ui::widgets
{
    TextInput::TextInput(wxWindow* parent) : parent_(parent)
    {
    }

    TextInput& TextInput::setHint(const wxString& hint)
    {
        hint_ = hint;
        return *this;
    }

    TextInput& TextInput::setValue(const wxString& value)
    {
        value_ = value;
        return *this;
    }

    TextInput& TextInput::setLines(int lines)
    {
        lines_ = lines;
        return *this;
    }

    TextInput& TextInput::setPasswordMode(bool state)
    {
        password_mode_ = state;
        return *this;
    }

    TextInput& TextInput::multiline()
    {
        multiline_ = true;
        return *this;
    }

    TextInput& TextInput::readOnly()
    {
        read_only_ = true;
        return *this;
    }

    TextInput& TextInput::onChange(std::function<void(wxTextCtrl&, const wxString&)> callback)
    {
        on_change_ = std::move(callback);
        return *this;
    }

    TextInput& TextInput::onEnter(std::function<void(wxTextCtrl&)> callback)
    {
        on_enter_ = std::move(callback);
        return *this;
    }

    wxTextCtrl* TextInput::build() const
    {
        long style = 0;

        if (password_mode_)
            style |= wxTE_PASSWORD;

        if (multiline_)
            style |= wxTE_MULTILINE;

        if (read_only_)
            style |= wxTE_READONLY;

        if (on_enter_)
            style |= wxTE_PROCESS_ENTER;

        auto* text = new wxTextCtrl(
            parent_,
            wxID_ANY,
            value_,
            wxDefaultPosition,
            wxDefaultSize,
            style);

        if (lines_ > 0)
        {
            const int height = text->GetSizeFromTextSize(
                                       text->GetCharHeight() * lines_)
                                   .y;

            text->SetMinSize(wxSize(-1, height));
        }

        if (!hint_.empty())
            text->SetHint(hint_);

        if (on_change_)
        {
            text->Bind(wxEVT_TEXT,
                       [callback = on_change_, text](wxCommandEvent&) {
                           callback(*text, text->GetValue());
                       });
        }

        if (on_enter_)
        {
            text->Bind(wxEVT_TEXT_ENTER,
                       [callback = on_enter_, text](wxCommandEvent&) {
                           callback(*text);
                       });
        }

        return text;
    }

} // namespace ui::widgets
