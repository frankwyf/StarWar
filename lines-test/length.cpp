#include <catch2/catch_amalgamated.hpp>

#include <algorithm>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE("Length of lines", "[Length]")
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
			{ 10.f, 400.f },
			{ 110.f, 400.f },
			{ 255, 255, 255 }
		);

		// length of line
		REQUIRE(max_row_pixel_count(surface) == 100);
	}
	SECTION("vertical")
	{
		draw_line_solid(surface,
			{ 110.f, 47.f },
			{ 110.f, 347.f },
			{ 255, 0, 0 }
		);
		draw_line_solid(surface,
			{ 81.f, 47.f },
			{ 81.f, 347.f },
			{ 255, 0, 0 }
		);
		draw_line_solid(surface,
			{ 209.f, 47.f },
			{ 209.f, 347.f },
			{ 255, 0, 0 }
		);

		REQUIRE(max_col_pixel_count(surface) == 300);
	}

	SECTION("diagonal")
	{
		draw_line_solid(surface,
			{ 10.f, 47.f },
			{ 110.f, 347.f },
			{ 0, 0, 255 }
		);

		REQUIRE(max_row_pixel_count(surface) == 1);
		REQUIRE(max_col_pixel_count(surface) == 3);
	}
}