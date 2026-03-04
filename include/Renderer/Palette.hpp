#pragma once

#include <Renderer/Color.hpp>

namespace renderer
{
    /**
     * A collection of predefined colors for consistent use throughout the application.
     */
    struct Palette
    {
        static constexpr Color Primary    = Color(0x4f, 0x5c, 0x7d); // #4f5c7d
        static constexpr Color Secondary  = Color(0x5e, 0x57, 0x68); // #5e5768
        static constexpr Color Accent     = Color(0xf9, 0xa3, 0x39); // #f9a339
        static constexpr Color Background = Color(0xe8, 0xe9, 0xf3); // #e8e9f3
        static constexpr Color Muted      = Color(0xb4, 0xb4, 0xb4); // #b4b4b4

        // Utility
        static constexpr Color Black       = Color(0, 0, 0);
        static constexpr Color White       = Color(255, 255, 255);
        static constexpr Color Transparent = Color(0, 0, 0, 0);
    };
} // namespace renderer