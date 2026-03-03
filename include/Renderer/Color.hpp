/*
** EPITECH PROJECT, 2025
** R-Type
** File description:
** Color.hpp
*/

#pragma once

#include <cstdint>

namespace renderer
{
    struct Color
    {
        uint8_t r, g, b, a;

        constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
            : r(r_), g(g_), b(b_), a(a_)
        { }

        static constexpr Color toColor(unsigned int rgba)
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
