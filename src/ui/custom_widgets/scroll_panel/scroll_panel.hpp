#pragma once
#include <set>
#include <utility>

#include <wx/panel.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Vertically scrollable container with a custom overlay scrollbar.
     */
    class ScrollPanel : public wxPanel
    {
      public:
        /**
         * @brief Fluent builder to configure and create a ScrollPanel.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Overrides the colours used to paint the panel and scrollbar.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Creates the ScrollPanel instance.
             */
            ScrollPanel* build() const;

          private:
            wxWindow* parent_  = nullptr;
            Palette   palette_ = Palette::Dark();
        };

        ScrollPanel(wxWindow* parent, Palette palette);

        /**
         * @brief Updates the colours used to paint the panel.
         */
        void setPalette(const Palette& palette);

        /**
         * @brief Returns the sizer for adding content controls.
         */
        wxSizer* contentSizer() const { return content_sizer_; }

        /**
         * @brief Returns the window to use as the parent of content controls.
         */
        wxWindow* contentParent() const { return content_panel_; }

        /**
         * @brief Recomputes the scrollable extent after content changes.
         */
        void refreshLayout();

      private:
        void onPaint(wxPaintEvent& event);
        void onMouseWheel(wxMouseEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void onLeftUp(wxMouseEvent& event);
        void onMotion(wxMouseEvent& event);
        void onSize(wxSizeEvent& event);

        void routeWheelEvents(wxWindow* window);

        int  maxScroll() const;
        void scrollTo(int offset);
        std::pair<int, int> thumbGeometry() const;

        Palette  palette_;
        wxPanel* content_panel_ = nullptr;
        wxSizer* content_sizer_ = nullptr;
        int      scroll_offset_ = 0;
        int      content_height_ = 0;
        bool     laying_out_     = false;
        wxSize   last_viewport_size_ = wxDefaultSize;
        bool     dragging_thumb_ = false;
        int      drag_start_y_    = 0;
        int      drag_start_offset_ = 0;
        int      drag_travel_      = 0;
        std::set<wxWindow*> wheel_routed_;

        static constexpr int kScrollbarWidth = 8;
        static constexpr int kMinThumbHeight = 28;
        static constexpr int kWheelStep      = 40;
    };

} // namespace ui::custom_widgets
