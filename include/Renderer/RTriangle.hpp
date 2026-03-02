#pragma once

#include <Renderer/Color.hpp>
#include <Renderer/RVec3.hpp>

namespace renderer
{
    struct RTriangle
    {
        RVec3 v0;
        RVec3 v1;
        RVec3 v2;
        Color color;
    };
} // namespace renderer
