#pragma once
#include <functional>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxTextCtrl.
     *
     * This class provides a chainable API to configure and create a wxTextCtrl.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* input = ui::widgets::TextInput(parent)
     *                   .setHint("Type here...")
     *                   .onChange([](wxTextCtrl& input, const wxString& value) {
     *                       wxLogMessage("Changed: %s", value);
     *                   })
     *                   .onEnter([](wxTextCtrl& input) {
     *                       wxLogMessage("Submitted: %s", input.GetValue());
     *                   })
     *                   .build();
     * @endcode
     */
    class TextInput
    {
    public:
        /**
         * @brief Creates a new TextInput builder.
         *
         * @param parent Parent wxWindow that will own the created text control.
         */
        explicit TextInput(wxWindow *parent);

        /**
         * @brief Sets the placeholder text displayed when the input is empty.
         *
         * @param hint Hint text displayed inside the control.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* input = ui::widgets::TextInput(parent)
         *                   .setHint("Enter your name")
         *                   .build();
         * @endcode
         */
        TextInput &setHint(const wxString &hint);

        /**
         * @brief Sets the initial text value of the input.
         *
         * @param value Initial text displayed by the control.
         *
         * @return Reference to this builder for method chaining.
         */
        TextInput &setValue(const wxString &value);

        /**
         * @brief Sets the number of visible text lines.
         *
         * This option is primarily intended for multiline text controls.
         * The final height is automatically calculated from the current font size.
         *
         * @param lines Number of visible text lines.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* input = ui::widgets::TextInput(parent)
         *                   .multiline()
         *                   .setLines(5)
         *                   .build();
         * @endcode
         *
         * @note
         * This setting has no visible effect unless multiline mode is enabled.
         */
        TextInput &setLines(int lines);

        /**
         * @brief Set password mode state.
         *
         * @param state State wanted. True enabled and False disabled
         *
         * Text entered by the user will be hidden.
         *
         * @return Reference to this builder for method chaining.
         */
        TextInput &setPasswordMode(bool state);

        /**
         * @brief Enables multiline text input.
         *
         * @return Reference to this builder for method chaining.
         */
        TextInput &multiline();

        /**
         * @brief Makes the text control read-only.
         *
         * Users can select and copy text but cannot modify its content.
         *
         * @return Reference to this builder for method chaining.
         */
        TextInput &readOnly();

        /**
         * @brief Registers a callback executed when the text changes.
         *
         * The callback receives:
         * - A reference to the created wxTextCtrl.
         * - The current text value.
         *
         * @param callback Function called on wxEVT_TEXT event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* input = ui::widgets::TextInput(parent)
         *     .onChange([](wxTextCtrl& input, const wxString& value) {
         *         wxLogMessage("Text: %s", value);
         *     })
         *     .build();
         * @endcode
         */
        TextInput &onChange(std::function<void(wxTextCtrl &, const wxString &)> callback);

        /**
         * @brief Registers a callback executed when Enter is pressed.
         *
         * @param callback Function called on wxEVT_TEXT_ENTER event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* input = ui::widgets::TextInput(parent)
         *     .onEnter([](wxTextCtrl& input) {
         *         wxLogMessage("Submitted: %s", input.GetValue());
         *     })
         *     .build();
         * @endcode
         *
         * @note
         * This option automatically enables wxTE_PROCESS_ENTER.
         */
        TextInput &onEnter(std::function<void(wxTextCtrl &)> callback);

        /**
         * @brief Creates the wxTextCtrl instance.
         *
         * The returned text control is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * Configured options such as password mode, multiline mode,
         * read-only mode and callbacks are applied during creation.
         *
         * @return Pointer to the created wxTextCtrl.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxTextCtrl *build() const;

    private:
        wxWindow *parent_ = nullptr;
        wxString hint_;
        wxString value_;
        int lines_ = 0;
        bool password_mode_ = false;
        bool multiline_ = false;
        bool read_only_ = false;
        std::function<void(wxTextCtrl &, const wxString &)> on_change_;
        std::function<void(wxTextCtrl &)> on_enter_;
    };

} // namespace ui::widgets
