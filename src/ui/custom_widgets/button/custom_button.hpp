#pragma once
#include <functional>
#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn push button.
     */
    class CustomButton : public wxControl
    {
      public:
        /**
         * @brief Fluent builder used to configure and create a CustomButton.
         */
        class Builder
        {
          public:
            /** @brief Creates a builder for the specified parent window. */
            explicit Builder(wxWindow* parent);

            /** @brief Sets the button label. */
            Builder& setLabel(const wxString& label);

            /** @brief Sets the hover tooltip text. */
            Builder& setTooltip(const wxString& tooltip);

            /** @brief Sets whether the button is enabled. */
            Builder& setEnabled(bool enabled);

            /** @brief Sets the colour palette. */
            Builder& setPalette(const Palette& palette);

            /** @brief Sets whether to use dark or light mode. */
            Builder& setDarkMode(bool dark_mode);

            /** @brief Sets the click callback. */
            Builder& onClick(std::function<void()> callback);

            /** @brief Creates the CustomButton instance. */
            CustomButton* build() const;

          private:
            wxWindow*             parent_ = nullptr;
            wxString              label_;
            wxString              tooltip_;
            bool                  enabled_ = true;
            Palette                palette_ = Palette::Dark();
            std::function<void()> on_click_;
        };

        CustomButton(wxWindow*              parent,
                     wxString               label,
                     Palette                palette,
                     std::function<void()>  on_click);

        /** @brief Updates the colour palette. */
        void setPalette(const Palette& palette);

        /** @brief Enables or disables the button. */
        bool Enable(bool enable = true) override;

      private:
        void onPaint(wxPaintEvent& event);
        void onMouseEnter(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void onLeftUp(wxMouseEvent& event);

        wxSize DoGetBestSize() const override;

        wxString              label_;
        Palette               palette_;
        std::function<void()> on_click_;
        bool                  hovered_ = false;
        bool                  pressed_ = false;
    };

} // namespace ui::custom_widgets
