#pragma once
#include <functional>
#include <wx/gauge.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxGauge.
     *
     * This class provides a chainable API to configure and create a wxGauge.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* gauge = ui::widgets::Gauge(parent)
     *                    .setRange(100)
     *                    .setValue(50)
     *                    .build();
     * @endcode
     */
    class Gauge
    {
      public:
        /**
         * @brief Creates a new Gauge builder.
         *
         * @param parent Parent wxWindow that will own the created gauge.
         */
        explicit Gauge(wxWindow* parent);

        /**
         * @brief Sets the maximum value of the gauge.
         *
         * The gauge value will be displayed as a percentage of this range.
         *
         * @param range Maximum gauge value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* gauge = ui::widgets::Gauge(parent)
         *                    .setRange(100)
         *                    .build();
         * @endcode
         */
        Gauge& setRange(int range);

        /**
         * @brief Sets the initial gauge value.
         *
         * @param value Initial gauge value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* gauge = ui::widgets::Gauge(parent)
         *                    .setValue(50)
         *                    .build();
         * @endcode
         */
        Gauge& setValue(int value);

        /**
         * @brief Creates the wxGauge instance.
         *
         * The returned gauge is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * The initial value configured with value() is applied after creation.
         *
         * @return Pointer to the created wxGauge.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxGauge* build() const;

      private:
        wxWindow* parent_ = nullptr;
        int       range_  = 100;
        int       value_  = 0;
    };

} // namespace ui::widgets
