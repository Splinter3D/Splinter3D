#pragma once

#include <cstdint>

namespace renderer
{
    /**
     * @brief A renderer-independent RGBA color.
     */
    struct Color
    {
        uint8_t r, g, b, a;

        constexpr Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
            : r(red)
            , g(green)
            , b(blue)
            , a(alpha)
        { }

        static constexpr Color fromRGBA(unsigned int rgba)
        {
            return Color{
                static_cast<uint8_t>((rgba >> 24) & 0xFF),
                static_cast<uint8_t>((rgba >> 16) & 0xFF),
                static_cast<uint8_t>((rgba >> 8) & 0xFF),
                static_cast<uint8_t>(rgba & 0xFF)};
        }

        constexpr unsigned int toRGBA() const
        {
            return (static_cast<unsigned int>(r) << 24) |
                   (static_cast<unsigned int>(g) << 16) |
                   (static_cast<unsigned int>(b) << 8) |
                   static_cast<unsigned int>(a);
        }
    };
} // namespace renderer
