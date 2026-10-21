#pragma once
#include <functional>

#include <wx/dialog.h>

#include "ui/custom_widgets/palette/palette.hpp"

namespace ui::custom_widgets
{
    /**
     * @brief Self-drawn modal dialog with a title, body area, and action buttons.
     */
    class Modal : public wxDialog
    {
      public:
        /**
         * @brief Fluent builder to configure and create a Modal.
         */
        class Builder
        {
          public:
            explicit Builder(wxWindow* parent);

            /**
             * @brief Sets the dialog title.
             */
            Builder& setTitle(const wxString& title);

            /**
             * @brief Sets the body text shown above the content area.
             */
            Builder& setMessage(const wxString& message);

            /**
             * @brief Sets the dialog size.
             */
            Builder& setSize(const wxSize& size);

            /**
             * @brief Overrides the colours used to paint the modal.
             */
            Builder& setPalette(const Palette& palette);

            /**
             * @brief Selects the dark or light built-in palette.
             */
            Builder& setDarkMode(bool dark_mode);

            /**
             * @brief Creates the Modal instance.
             */
            Modal* build() const;

          private:
            wxWindow* parent_ = nullptr;
            wxString  title_;
            wxString  message_;
            wxSize    size_    = wxSize(380, 180);
            Palette   palette_ = Palette::Dark();
        };

        Modal(wxWindow*       parent,
              const wxString& title,
              wxString        message,
              const wxSize&   size,
              Palette         palette);

        /**
         * @brief Appends a button to the action row.
         *
         * @param label Button text.
         * @param on_click Callback invoked before the modal closes.
         * @param primary Whether to paint the button in the accent colour.
         */
        void addAction(const wxString&       label,
                       std::function<void()> on_click,
                       bool                  primary = false);

        /**
         * @brief Shows the modal dialog.
         *
         * @return Dialog return code.
         */
        int showModal();

        /**
         * @brief Returns the sizer for adding body controls.
         */
        wxSizer* contentSizer() const { return content_sizer_; }

        /**
         * @brief Returns the parent window for body controls.
         */
        wxWindow* contentParent() const { return content_panel_; }

      private:
        void onPaint(wxPaintEvent& event);

        Palette  palette_;
        wxPanel* content_panel_ = nullptr;
        wxSizer* content_sizer_ = nullptr;
        wxSizer* actions_sizer_ = nullptr;

        static constexpr int kPadding = 16;
    };

} // namespace ui::custom_widgets
