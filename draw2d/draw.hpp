#ifndef DRAW_HPP_BA97BA20_4B0E_45D8_97D4_65267FFA2EA6
#define DRAW_HPP_BA97BA20_4B0E_45D8_97D4_65267FFA2EA6

// Public drawing API declarations used by game, tests and sandboxes.

#include "forward.hpp"
#include "color.hpp"

#include "../vmlib/vec2.hpp"

void draw_line_solid(
	Surface&,
	Vec2f aBegin, Vec2f aEnd,
	ColorU8_sRGB
);

void draw_triangle_solid(
	Surface& aSurface, 
	Vec2f aP0, Vec2f aP1, Vec2f aP2, 
	ColorU8_sRGB aColor);

void draw_triangle_interp(
	Surface&,
	Vec2f aP0, Vec2f aP1, Vec2f aP2,
	ColorF aC0, ColorF aC1, ColorF aC2
);

void draw_triangle_wireframe(
	Surface&,
	Vec2f aP0, Vec2f aP1, Vec2f aP2,
	ColorU8_sRGB
);

void draw_rectangle_solid(
	Surface&,
	Vec2f aMinCorner, Vec2f aMaxCorner,
	ColorU8_sRGB
);

void draw_rectangle_outline(
	Surface&,
	Vec2f aMinCorner, Vec2f aMaxCorner,
	ColorU8_sRGB
);

#endif // DRAW_HPP_BA97BA20_4B0E_45D8_97D4_65267FFA2EA6
