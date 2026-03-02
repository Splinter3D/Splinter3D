#pragma once

#include "renderer/RVec3.hpp"
#include "renderer/Color.hpp"

namespace renderer
{
    struct RTriangle {
        RVec3 v0; 
        RVec3 v1;
        RVec3 v2;
        Color color;
    };
}