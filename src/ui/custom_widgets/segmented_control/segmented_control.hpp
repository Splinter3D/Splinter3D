#pragma once
#include <functional>
#include <vector>

#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn exclusive-choice button group.
     */
    class SegmentedControl : public wxControl
    {
      public:
        /**
         * @brief Fluent builder to configure and create a SegmentedControl.
         */
        class Builder
        {
          public:
            /** @brief Creates a builder for the specified parent window. */
            explicit Builder(wxWindow* parent);

            /** @brief Sets the segment labels. */
            Builder& setSegments(std::vector<wxString> segments);

            /** @brief Sets the initially selected segment index. */
            Builder& setSelectedIndex(int index);

            /** @brief Sets the colour palette. */
            Builder& setPalette(const Palette& palette);

            /** @brief Sets whether to use dark or light mode. */
            Builder& setDarkMode(bool dark_mode);

            /** @brief Sets the selection change callback. */
            Builder& onSelectionChanged(std::function<void(int)> callback);

            /** @brief Creates the SegmentedControl instance. */
            SegmentedControl* build() const;

          private:
            wxWindow*                 parent_ = nullptr;
            std::vector<wxString>     segments_;
            int                       selected_index_ = 0;
            Palette                   palette_       = Palette::Dark();
            std::function<void(int)> on_selection_changed_;
        };

        SegmentedControl(wxWindow*                 parent,
                          std::vector<wxString>     segments,
                          int                       selected_index,
                          Palette                   palette,
                          std::function<void(int)> on_selection_changed);

        /** @brief Updates the colour palette. */
        void setPalette(const Palette& palette);

        /** @brief Returns the index of the currently selected segment. */
        int  selectedIndex() const { return selected_index_; }

      private:
        void onPaint(wxPaintEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void onMotion(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);

        int segmentAt(const wxPoint& pos) const;

        wxSize DoGetBestSize() const override;

        std::vector<wxString>     segments_;
        int                       selected_index_ = 0;
        int                       hovered_index_  = -1;
        Palette                   palette_;
        std::function<void(int)> on_selection_changed_;
    };

} // namespace ui::custom_widgets
