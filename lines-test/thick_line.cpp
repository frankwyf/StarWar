#include <catch2/catch_amalgamated.hpp>

#include <algorithm>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE("Adjacent lines", "[thick]")
{
	Surface surface(640, 480);
	surface.clear();

	SECTION("horizontal")
	{
		draw_line_solid(surface,
			{ 10.f, 47.f },
			{ 110.f, 47.f },
			{ 255, 255, 255 }
		);
		draw_line_solid(surface,
			{ 10.f, 48.f },
			{ 110.f, 48.f },
			{ 255, 255, 255 }
		);

		// horizontal line length is 101 pixels
		REQUIRE(max_col_pixel_count(surface) > 1);

		// thick line of two thin line
		REQUIRE(2 == max_col_pixel_count(surface));
	}
	SECTION("vertical")
	{
		draw_line_solid(surface,
			{ 110.f, 47.f },
			{ 110.f, 300.f },
			{ 255, 0, 0 }
		);
		draw_line_solid(surface,
			{ 111.f, 47.f },
			{ 111.f, 300.f },
			{ 0, 0, 255 }
		);
		draw_line_solid(surface,
			{109.f, 47.f },
			{109.f, 300.f },
			{ 255, 0, 0 }
		);

		REQUIRE(max_row_pixel_count(surface) == 3);
		REQUIRE(max_col_pixel_count(surface) > 1);
	}
}
