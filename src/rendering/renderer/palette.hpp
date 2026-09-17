#pragma once

#include <rendering/renderer/color.hpp>

namespace renderer
{
    struct Palette
    {
        static constexpr Color Primary    = Color(0x4f, 0x5c, 0x7d);
        static constexpr Color Secondary  = Color(0x5e, 0x57, 0x68);
        static constexpr Color Accent     = Color(0xf9, 0xa3, 0x39);
        static constexpr Color Background = Color(0xe8, 0xe9, 0xf3);
        static constexpr Color Muted      = Color(0xb4, 0xb4, 0xb4);

        static constexpr Color Black       = Color(0, 0, 0);
        static constexpr Color White       = Color(255, 255, 255);
        static constexpr Color Transparent = Color(0, 0, 0, 0);
    };
} // namespace renderer
