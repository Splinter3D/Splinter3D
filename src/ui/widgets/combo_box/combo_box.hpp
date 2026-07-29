#pragma once
#include <functional>
#include <wx/combobox.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a read-only wxComboBox.
     *
     * This class provides a chainable API to configure and create a
     * read-only wxComboBox.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * wxArrayString fruits;
     * fruits.Add("Apple");
     * fruits.Add("Banana");
     * fruits.Add("Orange");
     *
     * auto* combo = ui::widgets::ComboBox(parent)
     *                   .setPlaceholder("Pick a fruit")
     *                   .setChoices(fruits)
     *                   .onSelect([](wxComboBox& cb, const wxString& value) {
     *                       wxLogMessage("Selected: %s", value);
     *                   })
     *                   .build();
     * @endcode
     */
    class ComboBox
    {
    public:
        /**
         * @brief Creates a new ComboBox builder.
         *
         * @param parent Parent wxWindow that will own the created combo box.
         */
        explicit ComboBox(wxWindow *parent);

        /**
         * @brief Sets the placeholder text displayed before a selection is made.
         *
         * @param placeholder Placeholder text.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* combo = ui::widgets::ComboBox(parent)
         *                   .placeholder("Select an option")
         *                   .build();
         * @endcode
         */
        ComboBox &setPlaceholder(const wxString &placeholder);

        /**
         * @brief Sets the list of selectable items.
         *
         * @param choices Array of choices displayed in the combo box.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * wxArrayString items;
         * items.Add("One");
         * items.Add("Two");
         *
         * auto* combo = ui::widgets::ComboBox(parent)
         *                   .setChoices(items)
         *                   .build();
         * @endcode
         */
        ComboBox &setChoices(const wxArrayString &choices);

        /**
         * @brief Registers a callback executed when an item is selected.
         *
         * The callback receives:
         * - A reference to the created wxComboBox.
         * - The currently selected item.
         *
         * @param callback Function called on wxEVT_COMBOBOX event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* combo = ui::widgets::ComboBox(parent)
         *     .setChoices(items)
         *     .onSelect([](wxComboBox& cb, const wxString& value) {
         *         wxLogMessage("Selected: %s", value);
         *     })
         *     .build();
         * @endcode
         */
        ComboBox &onSelect(std::function<void(wxComboBox &, const wxString &)> callback);

        /**
         * @brief Creates the wxComboBox instance.
         *
         * The returned combo box is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a selection callback was registered, it will automatically be
         * bound to the created combo box.
         *
         * @return Pointer to the created wxComboBox.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxComboBox *build() const;

    private:
        wxWindow *parent_ = nullptr;
        wxString placeholder_;
        wxArrayString choices_;
        std::function<void(wxComboBox &, const wxString &)> on_select_;
    };

} // namespace ui::widgets
