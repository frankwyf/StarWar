#include <catch2/catch_amalgamated.hpp>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"



TEST_CASE("Adjacent triangles", "[mltiple triangles]")
{

	Surface surface(640, 480);
	surface.clear();

	SECTION("Triangle with extre height")
	{
		surface.fill({255, 255, 255});
		// testing window size
		draw_rectangle_outline(surface,
			{ 0.f, 0.f }, { 640.f, 480.f },
			{ 255, 255, 255 }
		);
		draw_triangle_solid(surface,
			{ 0.f, 0.f }, { 0.f, 480.f }, { 320.f, 480.f },
			{ 100, 100, 100 }
		);
		draw_triangle_solid(surface,
			{ 320.f, 480.f }, { 640.f, 480.f }, { 640.f, 0.f },
			{ 100, 100, 100 }
		);
		draw_triangle_solid(surface,
			{ 0.f, 0.f }, { 320.f, 0.f }, { 320.f, 480.f },
			{ 100, 100, 100 }
		);
		draw_triangle_solid(surface,
			{ 640.f, 0.f }, { 320.f, 480.f }, { 320.f, 0.f },
			{ 100, 100, 100 }
		);

		auto const col = find_least_red_nonzero_pixel(surface);
		REQUIRE(100 == int(col.r));
		REQUIRE(100 == int(col.r));
		REQUIRE(100 == int(col.r));
	}
}