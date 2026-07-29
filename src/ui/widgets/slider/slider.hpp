#pragma once
#include <functional>
#include <wx/slider.h>
#include <wx/wx.h>

namespace ui::widgets
{
    /**
     * @brief Fluent builder to create a wxSlider.
     *
     * This class provides a chainable API to configure and create a wxSlider.
     *
     * ---
     *
     * Example:
     *
     * @code{.cpp}
     * auto* slider = ui::widgets::Slider(parent)
     *                    .setRange(0, 100)
     *                    .setValue(50)
     *                    .withLabels()
     *                    .onChange([](wxSlider& slider, int value) {
     *                        wxLogMessage("Value: %d", value);
     *                    })
     *                    .build();
     * @endcode
     */
    class Slider
    {
    public:
        /**
         * @brief Creates a new Slider builder.
         *
         * @param parent Parent wxWindow that will own the created slider.
         */
        explicit Slider(wxWindow *parent);

        /**
         * @brief Sets the minimum and maximum values of the slider.
         *
         * @param min_range Minimum slider value.
         * @param max_range Maximum slider value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* slider = ui::widgets::Slider(parent)
         *                    .setRange(0, 100)
         *                    .build();
         * @endcode
         */
        Slider &setRange(int min_range, int max_range);

        /**
         * @brief Sets the initial slider value.
         *
         * @param value Initial slider value.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* slider = ui::widgets::Slider(parent)
         *                    .setValue(50)
         *                    .build();
         * @endcode
         */
        Slider &setValue(int value);

        /**
         * @brief Sets the slider vertical.
         *
         * By default, sliders are displayed horizontally.
         *
         * @return Reference to this builder for method chaining.
         */
        Slider &vertical();

        /**
         * @brief Displays value labels alongside the slider.
         *
         * When combined with a vertical slider, the highest value is displayed
         * at the top of the control.
         *
         * @return Reference to this builder for method chaining.
         */
        Slider &withLabels();

        /**
         * @brief Sets the length of the slider.
         *
         * For horizontal sliders, this corresponds to the width.
         * For vertical sliders, this corresponds to the height.
         *
         * @param length Length of the slider in pixels.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* slider = ui::widgets::Slider(parent)
         *                    .setLength(250)
         *                    .build();
         * @endcode
         */
        Slider &setLength(int length);

        /**
         * @brief Registers a callback executed when the slider value changes.
         *
         * The callback receives:
         * - A reference to the created wxSlider.
         * - The current slider value.
         *
         * @param callback Function called on wxEVT_SLIDER event.
         *
         * @return Reference to this builder for method chaining.
         *
         * ---
         *
         * Example:
         *
         * @code{.cpp}
         * auto* slider = ui::widgets::Slider(parent)
         *     .setRange(0, 100)
         *     .onChange([](wxSlider& slider, int value) {
         *         wxLogMessage("Current value: %d", value);
         *     })
         *     .build();
         * @endcode
         */
        Slider &onChange(std::function<void(wxSlider &, int)> callback);

        /**
         * @brief Creates the wxSlider instance.
         *
         * The returned slider is allocated dynamically and owned by
         * wxWidgets' parent-child lifetime management system.
         *
         * If a value change callback was registered, it will automatically be
         * bound to the created slider.
         *
         * @return Pointer to the created wxSlider.
         *
         * @warning The returned pointer must not be manually deleted.
         */
        wxSlider *build() const;

    private:
        wxWindow *parent_ = nullptr;
        int min_range_ = 0;
        int max_range_ = 100;
        int value_ = 0;
        int length_ = 100;
        bool vertical_ = false;
        bool with_labels_ = false;
        std::function<void(wxSlider &, int)> on_change_;
    };

} // namespace ui::widgets
