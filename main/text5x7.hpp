#pragma once

#include <string_view>

#include "../draw2d/color.hpp"
#include "../draw2d/surface.hpp"
#include "../vmlib/vec2.hpp"

namespace starwar
{
    void draw_text_5x7( Surface& s, Vec2f pos, int scale, ColorU8_sRGB col, std::string_view txt );
}
