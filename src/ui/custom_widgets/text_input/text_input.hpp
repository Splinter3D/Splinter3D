#pragma once
#include <functional>

#include <wx/panel.h>

#include "ui/custom_widgets/palette/palette.hpp"

class wxTextCtrl;

namespace ui::custom_widgets
{
    /**
     * @brief Editable text field with optional unit suffix.
     */
    class TextInput : public wxPanel
    {
      public:
        /**
         * @brief Fluent builder to configure and create a TextInput.
         */
        class Builder
        {
          public:
            /** @brief Creates a builder for the specified parent window. */
            explicit Builder(wxWindow* parent);

            /** @brief Sets the initial text content. */
            Builder& setValue(const wxString& value);

            /** @brief Sets the unit suffix displayed after the input field. */
            Builder& setSuffix(const wxString& suffix);

            /** @brief Sets the width in pixels. */
            Builder& setWidth(int width);

            /** @brief Sets the colour palette. */
            Builder& setPalette(const Palette& palette);

            /** @brief Sets whether to use dark or light mode. */
            Builder& setDarkMode(bool dark_mode);

            /** @brief Sets the text change callback. */
            Builder& onValueChanged(std::function<void(const wxString&)> callback);

            /** @brief Creates the TextInput instance. */
            TextInput* build() const;

          private:
            wxWindow*                              parent_ = nullptr;
            wxString                               value_;
            wxString                               suffix_;
            int                                    width_   = 72;
            Palette                                palette_ = Palette::Dark();
            std::function<void(const wxString&)> on_value_changed_;
        };

        TextInput(wxWindow*                              parent,
                  wxString                               value,
                  wxString                               suffix,
                  int                                    width,
                  Palette                                palette,
                  std::function<void(const wxString&)> on_value_changed);

        /** @brief Updates the colour palette. */
        void     setPalette(const Palette& palette);

        /** @brief Returns the current text content. */
        wxString value() const;

        /** @brief Sets the text content programmatically. */
        void     setValue(const wxString& value);

      private:
        void onPaint(wxPaintEvent& event);

        wxTextCtrl*                            text_ctrl_ = nullptr;
        wxString                               suffix_;
        Palette                                palette_;
        std::function<void(const wxString&)> on_value_changed_;

        static constexpr int kHeight   = 24;
        static constexpr int kPaddingX = 6;
    };

} // namespace ui::custom_widgets
