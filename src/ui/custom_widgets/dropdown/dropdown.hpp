#pragma once
#include <functional>
#include <vector>

#include <wx/control.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn dropdown field opening a custom popup list on click.
     */
    class Dropdown : public wxControl
    {
      public:
        /**
         * @brief Fluent builder to configure and create a Dropdown.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the choices offered by the dropdown.
             */
            Builder& setChoices(std::vector<wxString> choices);

            /**
             * @brief Sets the initially selected choice index.
             */
            Builder& setSelectedIndex(int index);

            /**
             * @brief Overrides the colours used to paint the dropdown.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Registers a callback executed when the selection changes.
             */
            Builder& onSelectionChanged(std::function<void(int)> callback);

            /**
             * @brief Creates the Dropdown instance.
             */
            Dropdown* build() const;

          private:
            wxWindow*                 parent_ = nullptr;
            std::vector<wxString>     choices_;
            int                       selected_index_ = 0;
            Palette                   palette_       = Palette::Dark();
            std::function<void(int)> on_selection_changed_;
        };

        Dropdown(wxWindow*                 parent,
                  std::vector<wxString>     choices,
                  int                       selected_index,
                  Palette                   palette,
                  std::function<void(int)> on_selection_changed);

        /**
         * @brief Updates the colours used to paint the dropdown.
         */
        void setPalette(const Palette& palette);

        /**
         * @brief Returns the index of the currently selected choice.
         */
        int  selectedIndex() const { return selected_index_; }

      private:
        class ListPopup;

        void onPaint(wxPaintEvent& event);
        void onMouseEnter(wxMouseEvent& event);
        void onMouseLeave(wxMouseEvent& event);
        void onLeftDown(wxMouseEvent& event);
        void openPopup();
        bool isPopupOpen() const;
        void selectIndex(int index);

        wxSize DoGetBestSize() const override;

        std::vector<wxString>     choices_;
        int                       selected_index_ = 0;
        Palette                   palette_;
        std::function<void(int)> on_selection_changed_;
        bool                       hovered_ = false;

        ListPopup*                 popup_ = nullptr;
    };

} // namespace ui::custom_widgets
