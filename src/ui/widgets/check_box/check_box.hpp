#pragma once
#include <functional>
#include <wx/checkbox.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxCheckBox.
     *
     * This class provides a chainable API to configure and create a wxCheckBox.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* checkBox = widgets::CheckBox(parent)
     *                      .setLabel("Enable feature")
     *                      .setChecked()
     *                      .onToggle([](wxCheckBox& cb, bool checked) {
     *                          cb.SetLabel(checked ? "Enabled" : "Disabled");
     *                      })
     *                      .build();
     * @endcode
     */
    class CheckBox
    {
    public:
        /**
         * @brief Creates a new CheckBox builder.
         *
         * @param parent Parent wxWindow that will own the created check box.
         */
        explicit CheckBox(wxWindow *parent);

        /**
         * @brief Sets the text displayed next to the check box.
         *
         * @param label Label displayed by the check box.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* checkBox = widgets::CheckBox(parent)
         *                      .setLabel("Enable feature")
         *                      .build();
         * @endcode
         */
        CheckBox &setLabel(const wxString &label);

        /**
         * @brief Sets the checked state.
         *
         * @param checked Initial checked state. Defaults to false.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* checkBox = widgets::CheckBox(parent)
         *                      .setChecked()
         *                      .build();
         * @endcode
         */
        CheckBox &setChecked(bool checked = false);

        /**
         * @brief Sets the checkbox state.
         *
         * @param state State wanted. True enabled and False disabled
         *
         * By default, check boxes are enabled.
         *
         * @return Reference to this builder for method chaining.
         */
        CheckBox &setEnabled(bool state = true);

        /**
         * @brief Registers a callback executed when the checked state changes.
         *
         * The callback receives:
         * - A reference to the created wxCheckBox.
         * - The current checked state of the check box.
         *
         * @param callback Function called on wxEVT_CHECKBOX event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* checkBox = widgets::CheckBox(parent)
         *     .setLabel("Enable feature")
         *     .onToggle([](wxCheckBox& cb, bool checked) {
         *         cb.SetLabel(checked ? "Enabled" : "Disabled");
         *     })
         *     .build();
         * @endcode
         */
        CheckBox &onToggle(std::function<void(wxCheckBox &, bool)> callback);

        /**
         * @brief Creates the wxCheckBox instance.
         *
         * The returned check box is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a toggle callback was registered, it will automatically be bound
         * to the created check box.
         *
         * @return Pointer to the created wxCheckBox.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxCheckBox *build() const;

    private:
        wxWindow *parent_ = nullptr;
        wxString label_;
        bool checked_ = false;
        bool enabled_ = true;
        std::function<void(wxCheckBox &, bool)> on_toggle_;
    };

} // namespace widgets
