#pragma once
#include <functional>
#include <wx/radiobox.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxRadioBox.
     *
     * This class provides a chainable API to configure and create a wxRadioBox.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * wxArrayString modes;
     * modes.Add("Easy");
     * modes.Add("Normal");
     * modes.Add("Hard");
     *
     * auto* radioBox = ui::widgets::RadioBox(parent)
     *                      .setTitle("Select mode")
     *                      .setChoices(modes)
     *                      .setColumns(1)
     *                      .onChange([](wxRadioBox& rb, const wxString& value) {
     *                          wxLogMessage("Selected: %s", value);
     *                      })
     *                      .build();
     * @endcode
     */
    class RadioBox
    {
      public:
        /**
         * @brief Creates a new RadioBox builder.
         *
         * @param parent Parent wxWindow that will own the created radio box.
         */
        explicit RadioBox(wxWindow* parent);

        /**
         * @brief Sets the title displayed above the radio box.
         *
         * @param title Title displayed by the radio box.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* radioBox = ui::widgets::RadioBox(parent)
         *                      .setTitle("Select mode")
         *                      .build();
         * @endcode
         */
        RadioBox& setTitle(const wxString& title);

        /**
         * @brief Sets the number of columns used to display choices.
         *
         * @param columns Number of columns.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* radioBox = ui::widgets::RadioBox(parent)
         *                      .setColumns(2)
         *                      .build();
         * @endcode
         */
        RadioBox& setColumns(int columns);

        /**
         * @brief Sets the available radio choices.
         *
         * @param choices Array of choices displayed in the radio box.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * wxArrayString choices;
         * choices.Add("Option A");
         * choices.Add("Option B");
         *
         * auto* radioBox = ui::widgets::RadioBox(parent)
         *                      .setChoices(choices)
         *                      .build();
         * @endcode
         */
        RadioBox& setChoices(const wxArrayString& choices);

        /**
         * @brief Registers a callback executed when the selected item changes.
         *
         * The callback receives:
         * - A reference to the created wxRadioBox.
         * - The currently selected item.
         *
         * @param callback Function called on wxEVT_RADIOBOX event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* radioBox = ui::widgets::RadioBox(parent)
         *     .setChoices(choices)
         *     .onChange([](wxRadioBox& rb, const wxString& value) {
         *         wxLogMessage("Selected: %s", value);
         *     })
         *     .build();
         * @endcode
         */
        RadioBox& onChange(std::function<void(wxRadioBox&, const wxString&)> callback);

        /**
         * @brief Creates the wxRadioBox instance.
         *
         * The returned radio box is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a selection callback was registered, it will automatically be
         * bound to the created radio box.
         *
         * @return Pointer to the created wxRadioBox.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxRadioBox* build() const;

      private:
        wxWindow*                                         parent_ = nullptr;
        wxString                                          title_;
        wxArrayString                                     choices_;
        int                                               columns_ = 1;
        std::function<void(wxRadioBox&, const wxString&)> on_change_;
    };

} // namespace ui::widgets
