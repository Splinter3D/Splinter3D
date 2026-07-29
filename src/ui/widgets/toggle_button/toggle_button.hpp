#pragma once
#include <functional>
#include <wx/tglbtn.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxToggleButton.
     *
     * This class provides a chainable API to configure and create a wxToggleButton.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* toggle = ui::widgets::ToggleButton(parent)
     *                    .setLabel("Toggle me")
     *                    .onToggle([](wxToggleButton& btn, bool enabled) {
     *                        btn.SetLabel(enabled ? "Enabled" : "Disabled");
     *                    })
     *                    .build();
     * @endcode
     */
    class ToggleButton
    {
    public:
        /**
         * @brief Creates a new ToggleButton builder.
         *
         * @param parent Parent wxWindow that will own the created toggle button.
         */
        explicit ToggleButton(wxWindow *parent);

        /**
         * @brief Sets the text displayed on the toggle button.
         *
         * @param label Label displayed by the toggle button.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* toggle = ui::widgets::ToggleButton(parent)
         *                    .setLabel("Enable feature")
         *                    .build();
         * @endcode
         */
        ToggleButton &setLabel(const wxString &label);

        /**
         * @brief Registers a callback executed when the toggle state changes.
         *
         * The callback receives:
         * - A reference to the created wxToggleButton.
         * - The current checked state of the button.
         *
         * @param callback Function called on wxEVT_TOGGLEBUTTON event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* toggle = ui::widgets::ToggleButton(parent)
         *     .setLabel("Toggle me OFF")
         *     .onToggle([](wxToggleButton& btn, bool enabled) {
         *         btn.SetLabel(enabled ? "Toggle me ON" : "Toggle me OFF");
         *     })
         *     .build();
         * @endcode
         */
        ToggleButton &onToggle(std::function<void(wxToggleButton &, bool)> callback);

        /**
         * @brief Creates the wxToggleButton instance.
         *
         * The returned button is allocated dynamically and owned by wxWidgets'
         * parent-child lifetime management system.
         *
         * If a toggle callback was registered, it will automatically be bound
         * to the created button.
         *
         * @return Pointer to the created wxToggleButton.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxToggleButton *build() const;

    private:
        wxWindow *parent_ = nullptr;
        wxString label_;
        std::function<void(wxToggleButton &, bool)> on_toggle_;
    };

} // namespace ui::widgets
