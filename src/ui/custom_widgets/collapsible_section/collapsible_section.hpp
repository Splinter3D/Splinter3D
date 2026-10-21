#pragma once
#include <functional>
#include <wx/panel.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn foldable group with a clickable header and content area.
     */
    class CollapsibleSection : public wxPanel
    {
      public:
        /**
         * @brief Fluent builder to configure and create a CollapsibleSection.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the title shown in the header.
             */
            Builder& setTitle(const wxString& title);

            /**
             * @brief Sets whether the section starts expanded.
             */
            Builder& setExpanded(bool expanded);

            /**
             * @brief Overrides the colours used to paint the section.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Creates the CollapsibleSection instance.
             */
            CollapsibleSection* build() const;

          private:
            wxWindow* parent_   = nullptr;
            wxString  title_;
            bool      expanded_ = true;
            Palette   palette_  = Palette::Dark();
        };

        CollapsibleSection(wxWindow* parent, wxString title, bool expanded, Palette palette);

        /**
         * @brief Updates the colours used to paint the section.
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
         * @brief Returns whether the content area is currently visible.
         */
        bool isExpanded() const { return expanded_; }

        /**
         * @brief Expands or collapses the section.
         */
        void setExpanded(bool expanded);

        /**
         * @brief Registers a callback executed after the section is toggled.
         */
        void setOnToggled(std::function<void(bool)> callback);

      private:
        class HeaderPanel;

        wxString     title_;
        bool         expanded_ = true;
        Palette      palette_;
        HeaderPanel* header_panel_  = nullptr;
        wxPanel*     content_panel_ = nullptr;
        wxSizer*     content_sizer_ = nullptr;

        std::function<void(bool)> on_toggled_;

        static constexpr int kHeaderHeight = 28;
    };

} // namespace ui::custom_widgets
