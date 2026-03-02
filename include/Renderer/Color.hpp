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
} // namespace renderer
