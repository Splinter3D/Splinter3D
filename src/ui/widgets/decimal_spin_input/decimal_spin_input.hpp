#pragma once
#include <functional>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxSpinCtrlDouble.
     *
     * Same idea as ui::widgets::SpinInput, but for decimal values (e.g.
     * millimeters or degrees) instead of integers.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* spin = ui::widgets::DecimalSpinInput(parent)
     *                  .setRange(-1000.0, 1000.0)
     *                  .setIncrement(0.1)
     *                  .setDigits(2)
     *                  .setValue(0.0)
     *                  .onChange([](wxSpinCtrlDouble& spin, double value) {
     *                      wxLogMessage("Value: %f", value);
     *                  })
     *                  .build();
     * @endcode
     */
    class DecimalSpinInput
    {
      public:
        /**
         * @brief Creates a new DecimalSpinInput builder.
         *
         * @param parent Parent wxWindow that will own the created spin control.
         */
        explicit DecimalSpinInput(wxWindow* parent);

        /**
         * @brief Sets the minimum and maximum values of the spin control.
         *
         * @return Reference to this builder for method chaining.
         */
        DecimalSpinInput& setRange(double min_range, double max_range);

        /**
         * @brief Sets the initial value of the spin control.
         *
         * @return Reference to this builder for method chaining.
         */
        DecimalSpinInput& setValue(double value);

        /**
         * @brief Sets the step applied by the spin arrows.
         *
         * @return Reference to this builder for method chaining.
         */
        DecimalSpinInput& setIncrement(double increment);

        /**
         * @brief Sets the number of decimal digits displayed.
         *
         * @return Reference to this builder for method chaining.
         */
        DecimalSpinInput& setDigits(int digits);

        /**
         * @brief Registers a callback executed when the value changes.
         *
         * @param callback Function called on wxEVT_SPINCTRLDOUBLE.
         *
         * @return Reference to this builder for method chaining.
         */
        DecimalSpinInput& onChange(std::function<void(wxSpinCtrlDouble&, double)> callback);

        /**
         * @brief Creates the wxSpinCtrlDouble instance.
         *
         * The returned spin control is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * @return Pointer to the created wxSpinCtrlDouble.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxSpinCtrlDouble* build() const;

      private:
        wxWindow*                                      parent_    = nullptr;
        double                                         min_range_ = 0.0;
        double                                         max_range_ = 100.0;
        double                                         value_     = 0.0;
        double                                         increment_ = 1.0;
        int                                            digits_    = 2;
        std::function<void(wxSpinCtrlDouble&, double)> on_change_;
    };

} // namespace ui::widgets
