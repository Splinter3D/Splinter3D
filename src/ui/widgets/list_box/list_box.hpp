#pragma once
#include <functional>
#include <wx/listbox.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxListBox.
     *
     * This class provides a chainable API to configure and create a wxListBox.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * wxArrayString languages;
     * languages.Add("C++");
     * languages.Add("Python");
     * languages.Add("Rust");
     *
     * auto* listBox = ui::widgets::ListBox(parent)
     *                     .setChoices(languages)
     *                     .multiSelection(true)
     *                     .setRows(120)
     *                     .onSelect([](wxListBox& lb, int index) {
     *                         wxLogMessage("Selected: %s", lb.GetString(index));
     *                     })
     *                     .build();
     * @endcode
     */
    class ListBox
    {
      public:
        /**
         * @brief Creates a new ListBox builder.
         *
         * @param parent Parent wxWindow that will own the created list box.
         */
        explicit ListBox(wxWindow* parent);

        /**
         * @brief Sets the items displayed in the list box.
         *
         * @param choices Array of items displayed in the list box.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * wxArrayString items;
         * items.Add("First");
         * items.Add("Second");
         *
         * auto* listBox = ui::widgets::ListBox(parent)
         *                     .setChoices(items)
         *                     .build();
         * @endcode
         */
        ListBox& setChoices(const wxArrayString& choices);

        /**
         * @brief Sets the multiSelection state.
         *
         * By default, the list box allows selecting only one item.
         *
         * @return Reference to this builder for method chaining.
         */
        ListBox& multiSelection();

        /**
         * @brief Sets the number of visible rows in the list box.
         *
         * The final height is automatically calculated from the current font size.
         * This makes the widget adapt better to DPI scaling and platform themes.
         *
         * @param rows Number of visible rows.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* listBox = ui::widgets::ListBox(parent)
         *                     .setRows(5)
         *                     .build();
         * @endcode
         */
        ListBox& setRows(int rows);

        /**
         * @brief Registers a callback executed when an item is selected.
         *
         * The callback receives:
         * - A reference to the created wxListBox.
         * - The index of the selected item.
         *
         * @param callback Function called on wxEVT_LISTBOX event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* listBox = ui::widgets::ListBox(parent)
         *     .setChoices(items)
         *     .onSelect([](wxListBox& lb, int index) {
         *         wxLogMessage("Selected: %s", lb.GetString(index));
         *     })
         *     .build();
         * @endcode
         *
         * @note
         * In multi-selection mode, the callback is invoked whenever the selection
         * changes. Use wxListBox::GetSelections() to retrieve all selected items.
         */
        ListBox& onSelect(std::function<void(wxListBox&, int)> callback);

        /**
         * @brief Creates the wxListBox instance.
         *
         * The returned list box is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a selection callback was registered, it will automatically be bound
         * to the created list box.
         *
         * @return Pointer to the created wxListBox.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxListBox* build() const;

      private:
        wxWindow*                            parent_ = nullptr;
        wxArrayString                        choices_;
        bool                                 multi_selection_ = false;
        int                                  rows_            = 0;
        std::function<void(wxListBox&, int)> on_select_;
    };

} // namespace ui::widgets
