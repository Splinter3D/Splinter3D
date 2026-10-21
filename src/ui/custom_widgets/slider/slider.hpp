#pragma once
#include <functional>

#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn horizontal slider control.
     */
    class Slider : public wxControl
    {
      public:
        /**
         * @brief Fluent builder to configure and create a Slider.
         */
        class Builder
        {
          public:
            /** @brief Creates a builder for the specified parent window. */
            explicit Builder(wxWindow* parent);

            /** @brief Sets the minimum and maximum values. */
            Builder& setRange(int min_value, int max_value);

            /** @brief Sets the initial value. */
            Builder& setValue(int value);

            /** @brief Sets whether the slider is enabled. */
            Builder& setEnabled(bool enabled);

            /** @brief Sets the colour palette. */
            Builder& setPalette(const Palette& palette);

            /** @brief Sets whether to use dark or light mode. */
            Builder& setDarkMode(bool dark_mode);

            /** @brief Sets the value change callback. */
            Builder& onValueChanged(std::function<void(int)> callback);

            /** @brief Creates the Slider instance. */
            Slider* build() const;

          private:
            wxWindow*                 parent_   = nullptr;
            int                       min_value_ = 0;
            int                       max_value_ = 100;
            int                       value_    = 0;
            bool                      enabled_  = true;
            Palette                   palette_  = Palette::Dark();
            std::function<void(int)> on_value_changed_;
        };

        Slider(wxWindow*                 parent,
               int                       min_value,
               int                       max_value,
               int                       value,
               Palette                   palette,
               std::function<void(int)> on_value_changed);

        /** @brief Updates the colour palette. */
        void setPalette(const Palette& palette);

        /** @brief Returns the current value. */
        int  value() const { return value_; }

        /** @brief Sets the value programmatically. */
        void setValue(int value);

        /** @brief Enables or disables the control. */
        bool Enable(bool enable = true) override;

      private:
        void onPaint(wxPaintEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void onLeftUp(wxMouseEvent& event);
        void onMotion(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);

        int  valueAt(int x) const;
        void applyValueFromPointer(int x);
        double knobCentreX() const;

        wxSize DoGetBestSize() const override;

        int                       min_value_ = 0;
        int                       max_value_ = 100;
        int                       value_    = 0;
        bool                      hovered_  = false;
        bool                      dragging_ = false;
        Palette                   palette_;
        std::function<void(int)> on_value_changed_;

        static constexpr int kHeight      = 24;
        static constexpr int kTrackHeight = 4;
        static constexpr int kKnobRadius  = 7;
    };

} // namespace ui::custom_widgets
