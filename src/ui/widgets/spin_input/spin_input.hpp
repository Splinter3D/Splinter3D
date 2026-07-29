#pragma once
#include <functional>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxSpinCtrl.
     *
     * This class provides a chainable API to configure and create a wxSpinCtrl.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* spin = ui::widgets::SpinInput(parent)
     *                  .setRange(0, 200)
     *                  .setValue(42)
     *                  .onChange([](wxSpinCtrl& spin, int value) {
     *                      wxLogMessage("Value: %d", value);
     *                  })
     *                  .build();
     * @endcode
     */
    class SpinInput
    {
    public:
        /**
         * @brief Creates a new SpinInput builder.
         *
         * @param parent Parent wxWindow that will own the created spin control.
         */
        explicit SpinInput(wxWindow *parent);

        /**
         * @brief Sets the minimum and maximum values of the spin control.
         *
         * @param min_range Minimum allowed value.
         * @param max_range Maximum allowed value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* spin = ui::widgets::SpinInput(parent)
         *                  .setRange(0, 200)
         *                  .build();
         * @endcode
         */
        SpinInput &setRange(int min_range, int max_range);

        /**
         * @brief Sets the initial value of the spin control.
         *
         * @param value Initial value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* spin = ui::widgets::SpinInput(parent)
         *                  .setValue(42)
         *                  .build();
         * @endcode
         */
        SpinInput &setValue(int value);

        /**
         * @brief Registers a callback executed when the value changes.
         *
         * The callback receives:
         * - A reference to the created wxSpinCtrl.
         * - The current value of the spin control.
         *
         * @param callback Function called on wxEVT_SPINCTRL event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* spin = ui::widgets::SpinInput(parent)
         *     .setRange(0, 100)
         *     .onChange([](wxSpinCtrl& spin, int value) {
         *         wxLogMessage("Current value: %d", value);
         *     })
         *     .build();
         * @endcode
         */
        SpinInput &onChange(std::function<void(wxSpinCtrl &, int)> callback);

        /**
         * @brief Creates the wxSpinCtrl instance.
         *
         * The returned spin control is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a value change callback was registered, it will automatically be
         * bound to the created spin control.
         *
         * @return Pointer to the created wxSpinCtrl.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxSpinCtrl *build() const;

    private:
        wxWindow *parent_ = nullptr;
        int min_range_ = 0;
        int max_range_ = 100;
        int value_ = 0;
        std::function<void(wxSpinCtrl &, int)> on_change_;
    };

} // namespace ui::widgets
