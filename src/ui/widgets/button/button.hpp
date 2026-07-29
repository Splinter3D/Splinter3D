#pragma once
#include <functional>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxButton.
     *
     * This class provides a chainable API to configure and create a wxButton.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* btn = ui::widgets::Button(parent)
     *                 .setLabel("Click me")
     *                 .setTooltip("Press this button")
     *                 .onClick([] { wxMessageBox("Hello!"); })
     *                 .build();
     * @endcode
     */
    class Button
    {
    public:
        /**
         * @brief Creates a new Button builder.
         *
         * @param parent Parent wxWindow that will own the created button.
         */
        explicit Button(wxWindow *parent);

        /**
         * @brief Sets the text displayed on the button.
         *
         * @param label Label displayed by the button.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code
         * auto* btn = ui::widgets::Button(parent)
         *                 .setLabel("Save")
         *                 .build();
         * @endcode
         */
        Button &setLabel(const wxString &label);

        /**
         * @brief Sets the tooltip displayed when hovering the button.
         *
         * @param tooltip Tooltip text.
         *
         * @return Reference to this builder for method chaining.
         */
        Button &setTooltip(const wxString &tooltip);

        /**
         * @brief Sets the button state.
         *
         * @param state State wanted. True enabled and False disabled
         *
         * By default, buttons are enabled.
         *
         * @return Reference to this builder for method chaining.
         */
        Button &setEnabled(bool state);

        /**
         * @brief Registers a callback executed when the button is clicked.
         *
         * @param callback Function called on wxEVT_BUTTON event.
         *
         * @return Reference to this builder for method chaining.
         */
        Button &onClick(std::function<void()> callback);

        /**
         * @brief Creates the wxButton instance.
         *
         * The returned button is allocated dynamically and owned by wxWidgets'
         * parent-child lifetime management system.
         *
         * @return Pointer to the created wxButton.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxButton *build() const;

    private:
        wxWindow *parent_ = nullptr;
        wxString label_;
        wxString tooltip_;
        bool enabled_ = true;
        std::function<void()> on_click_;
    };

} // namespace ui::widgets
