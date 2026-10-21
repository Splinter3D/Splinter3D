#pragma once
#include <functional>
#include <vector>
#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn horizontal tab strip with an active-tab indicator.
     */
    class TabBar : public wxControl
    {
      public:
        /**
         * @brief Fluent builder to configure and create a TabBar.
         */
        class Builder
        {
          public:
            /** @brief Creates a builder for the specified parent window. */
            explicit Builder(wxWindow* parent);

            /** @brief Sets the tab labels. */
            Builder& setTabs(std::vector<wxString> tabs);

            /** @brief Sets the initially selected tab index. */
            Builder& setSelectedIndex(int index);

            /** @brief Sets the colour palette. */
            Builder& setPalette(const Palette& palette);

            /** @brief Sets whether to use dark or light mode. */
            Builder& setDarkMode(bool dark_mode);

            /** @brief Sets the tab change callback. */
            Builder& onTabChanged(std::function<void(int)> callback);

            /** @brief Creates the TabBar instance. */
            TabBar* build() const;

          private:
            wxWindow*                parent_         = nullptr;
            std::vector<wxString>    tabs_;
            int                      selected_index_ = 0;
            Palette                  palette_        = Palette::Dark();
            std::function<void(int)> on_tab_changed_;
        };

        TabBar(wxWindow*                parent,
               std::vector<wxString>    tabs,
               int                      selected_index,
               Palette                  palette,
               std::function<void(int)> on_tab_changed);

        /** @brief Updates the colour palette. */
        void setPalette(const Palette& palette);

        /** @brief Sets the active tab index programmatically. */
        void setSelectedIndex(int index);

        /** @brief Returns the index of the currently active tab. */
        int selectedIndex() const { return selected_index_; }

      private:
        void onPaint(wxPaintEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void onMotion(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);

        int tabAt(const wxPoint& pos) const;

        wxSize DoGetBestSize() const override;

        std::vector<wxString>    tabs_;
        int                      selected_index_ = 0;
        int                      hovered_index_  = -1;
        Palette                  palette_;
        std::function<void(int)> on_tab_changed_;

        static constexpr int kTabPaddingX     = 18;
        static constexpr int kIndicatorHeight = 3;
    };

} // namespace ui::custom_widgets
