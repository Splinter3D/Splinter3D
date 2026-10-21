#pragma once
#include <functional>

#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn pill-shaped on/off switch with a sliding knob.
     */
    class ToggleSwitch : public wxControl
    {
      public:
        /**
         * @brief Fluent builder to configure and create a ToggleSwitch.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the initial checked state.
             */
            Builder& setChecked(bool checked);

            /**
             * @brief Sets whether the switch accepts user interaction.
             */
            Builder& setEnabled(bool enabled);

            /**
             * @brief Overrides the colours used to paint the switch.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Registers a callback executed when the user flips the switch.
             */
            Builder& onToggled(std::function<void(bool)> callback);

            /**
             * @brief Creates the ToggleSwitch instance.
             */
            ToggleSwitch* build() const;

          private:
            wxWindow*                  parent_  = nullptr;
            bool                       checked_ = false;
            bool                       enabled_ = true;
            Palette                    palette_ = Palette::Dark();
            std::function<void(bool)> on_toggled_;
        };

        ToggleSwitch(wxWindow*                  parent,
                     bool                       checked,
                     Palette                    palette,
                     std::function<void(bool)> on_toggled);

        /**
         * @brief Updates the colours used to paint the switch.
         */
        void setPalette(const Palette& palette);

        /**
         * @brief Returns the current checked state.
         */
        bool isChecked() const { return checked_; }

        /**
         * @brief Sets the checked state programmatically.
         */
        void setChecked(bool checked);

        /**
         * @brief Enables or disables the switch.
         */
        bool Enable(bool enable = true) override;

      private:
        void onPaint(wxPaintEvent& event);
        void onLeftUp(wxMouseEvent& event);
        void onMouseEnter(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);

        wxSize DoGetBestSize() const override;

        bool                       checked_ = false;
        bool                       hovered_ = false;
        Palette                    palette_;
        std::function<void(bool)> on_toggled_;
    };

} // namespace ui::custom_widgets
