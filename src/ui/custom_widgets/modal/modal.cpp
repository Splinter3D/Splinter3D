#include "modal.hpp"

#include <memory>

#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

#include "ui/custom_widgets/button/custom_button.hpp"

namespace ui::custom_widgets
{
    Modal::Builder::Builder(wxWindow* parent) : parent_(parent) {}

    Modal::Builder& Modal::Builder::setTitle(const wxString& title)
    {
        title_ = title;
        return *this;
    }

    Modal::Builder& Modal::Builder::setMessage(const wxString& message)
    {
        message_ = message;
        return *this;
    }

    Modal::Builder& Modal::Builder::setSize(const wxSize& size)
    {
        size_ = size;
        return *this;
    }

    Modal::Builder& Modal::Builder::setPalette(const Palette& palette)
    {
        palette_ = palette;
        return *this;
    }

    Modal::Builder& Modal::Builder::setDarkMode(bool dark_mode)
    {
        palette_ = dark_mode ? Palette::Dark() : Palette::Light();
        return *this;
    }

    Modal* Modal::Builder::build() const
    {
        return new Modal(parent_, title_, message_, size_, palette_);
    }

    Modal::Modal(wxWindow*       parent,
                 const wxString& title,
                 wxString        message,
                 const wxSize&   size,
                 Palette         palette)
        : wxDialog(parent, wxID_ANY, title, wxDefaultPosition, size,
                   wxDEFAULT_DIALOG_STYLE & ~wxRESIZE_BORDER),
          palette_(std::move(palette))
    {
        SetBackgroundColour(palette_.background);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &Modal::onPaint, this);

        wxBoxSizer* rootSizer = new wxBoxSizer(wxVERTICAL);

        if (!message.empty())
        {
            wxStaticText* messageLabel = new wxStaticText(this, wxID_ANY, message);
            messageLabel->SetForegroundColour(palette_.text);
            messageLabel->Wrap(size.x - 2 * kPadding);
            rootSizer->Add(messageLabel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, kPadding);
        }

        content_panel_ = new wxPanel(this, wxID_ANY);
        content_panel_->SetBackgroundColour(palette_.background);
        content_sizer_ = new wxBoxSizer(wxVERTICAL);
        content_panel_->SetSizer(content_sizer_);
        rootSizer->Add(content_panel_, 1, wxEXPAND | wxALL, kPadding);

        actions_sizer_ = new wxBoxSizer(wxHORIZONTAL);
        actions_sizer_->AddStretchSpacer(1);
        rootSizer->Add(actions_sizer_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, kPadding);

        SetSizer(rootSizer);
    }

    void Modal::addAction(const wxString& label, std::function<void()> on_click, bool primary)
    {
        Palette buttonPalette = palette_;
        if (primary)
        {
            buttonPalette.background        = palette_.accent;
            buttonPalette.backgroundHover   = palette_.accentHover;
            buttonPalette.backgroundPressed = palette_.accentPressed;
            buttonPalette.border            = palette_.accent;
            buttonPalette.text              = palette_.onAccent;
        }

        CustomButton* button = CustomButton::Builder(this)
                            .setLabel(label)
                            .setPalette(buttonPalette)
                            .onClick([this, on_click = std::move(on_click)] {
                                if (on_click)
                                {
                                    on_click();
                                }
                                EndModal(wxID_OK);
                            })
                            .build();

        actions_sizer_->Add(button, 0, wxLEFT, 8);
        Layout();
    }

    int Modal::showModal()
    {
        Layout();
        CentreOnParent();
        return ShowModal();
    }

    void Modal::onPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc)
        {
            return;
        }

        const wxSize size = GetClientSize();

        gc->SetBrush(wxBrush(palette_.background));
        gc->SetPen(wxPen(palette_.border, 1));
        gc->DrawRectangle(0, 0, size.x, size.y);
    }

} // namespace ui::custom_widgets
