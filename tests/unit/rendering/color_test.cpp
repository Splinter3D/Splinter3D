#include "rendering/renderer/color.hpp"
#include "rendering/renderer/palette.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using renderer::Color;
using renderer::Palette;

static_assert(Color(1, 2, 3).a == 255, "alpha defaults to opaque");
static_assert(Color::fromRGBA(0x11223344).toRGBA() == 0x11223344);
static_assert(Palette::Transparent.a == 0);

TEST_CASE("Color: fromRGBA splits channels most significant first", "[rendering][color]")
{
    const Color c = Color::fromRGBA(0xf9a33980);

    CHECK(c.r == 0xf9);
    CHECK(c.g == 0xa3);
    CHECK(c.b == 0x39);
    CHECK(c.a == 0x80);
}

TEST_CASE("Color: toRGBA and fromRGBA round-trip", "[rendering][color]")
{
    const unsigned int rgba = GENERATE(0x00000000u, 0xffffffffu, 0x4f5c7dffu, 0x01020304u);
    CAPTURE(rgba);

    REQUIRE(Color::fromRGBA(rgba).toRGBA() == rgba);
}

TEST_CASE("Palette: named colors match their documented hex values", "[rendering][palette]")
{
    CHECK(Palette::Primary.toRGBA() == 0x4f5c7dffu);
    CHECK(Palette::Accent.toRGBA() == 0xf9a339ffu);
    CHECK(Palette::Black.toRGBA() == 0x000000ffu);
    CHECK(Palette::White.toRGBA() == 0xffffffffu);
}
