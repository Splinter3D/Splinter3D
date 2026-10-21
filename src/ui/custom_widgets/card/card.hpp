#pragma once
#include <wx/panel.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn rounded container panel with an optional title.
     */
    class Card : public wxPanel
    {
      public:
        /**
         * @brief Fluent builder to configure and create a Card.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the title shown in the card's header band.
             */
            Builder& setTitle(const wxString& title);

            /**
             * @brief Overrides the colours used to paint the card.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Creates the Card instance.
             */
            Card* build() const;

          private:
            wxWindow* parent_ = nullptr;
            wxString  title_;
            Palette   palette_ = Palette::Dark();
        };

        Card(wxWindow* parent, wxString title, Palette palette);

        /**
         * @brief Updates the colours used to paint the card.
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

      private:
        class HeaderPanel;

        void onPaint(wxPaintEvent& event);

        wxString     title_;
        Palette      palette_;
        HeaderPanel* header_panel_  = nullptr;
        wxPanel*     content_panel_ = nullptr;
        wxSizer*     content_sizer_ = nullptr;

        static constexpr int kTitleBandHeight = 32;
        static constexpr int kPadding         = 10;
    };

} // namespace ui::custom_widgets
