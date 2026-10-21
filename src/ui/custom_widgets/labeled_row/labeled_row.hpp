#pragma once
#include <functional>

#include <wx/panel.h>

#include "ui/custom_widgets/palette/palette.hpp"
#include "ui/custom_widgets/text_input/text_input.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Settings row with a label on the left and an editable field on the right.
     */
    class LabeledRow : public wxPanel
    {
      public:
        /**
         * @brief Fluent builder to configure and create a LabeledRow.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the label text.
             */
            Builder& setLabel(const wxString& label);

            /**
             * @brief Sets the initial text of the value field.
             */
            Builder& setValue(const wxString& value);

            /**
             * @brief Sets the unit suffix displayed after the value.
             */
            Builder& setSuffix(const wxString& suffix);

            /**
             * @brief Overrides the colours used to paint the row.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Registers a callback executed when the value is edited.
             */
            Builder& onValueChanged(std::function<void(const wxString&)> callback);

            /**
             * @brief Creates the LabeledRow instance.
             */
            LabeledRow* build() const;

          private:
            wxWindow*                              parent_ = nullptr;
            wxString                               label_;
            wxString                               value_;
            wxString                               suffix_;
            Palette                                palette_ = Palette::Dark();
            std::function<void(const wxString&)> on_value_changed_;
        };

        LabeledRow(wxWindow*                              parent,
                   wxString                               label,
                   wxString                               value,
                   wxString                               suffix,
                   Palette                                palette,
                   std::function<void(const wxString&)> on_value_changed);

        /**
         * @brief Updates the colours used to paint the row.
         */
        void     setPalette(const Palette& palette);

        /**
         * @brief Returns the current text of the value field.
         */
        wxString value() const;

        /**
         * @brief Sets the text of the value field programmatically.
         */
        void     setValue(const wxString& value);

      private:
        class LabelPanel;

        LabelPanel* label_panel_ = nullptr;
        TextInput*  input_      = nullptr;
        Palette     palette_;

        static constexpr int kRowHeight = 28;
    };

} // namespace ui::custom_widgets
