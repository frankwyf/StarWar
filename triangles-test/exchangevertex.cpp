#include <catch2/catch_amalgamated.hpp>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE("Exchange vertex", "[Overlapping]")
{
	Surface surface(640, 480);
	surface.clear();
	// We'd expected that draw_triangle_solid() draws the same triangle as
	// draw_triangle_interp()!

	SECTION("Overlapping solid")
	{
		// Draw over same triangle
		draw_triangle_solid(surface,
			{ 60.f, 5.f }, { 300.f, 150.f }, { 77.f, 310.f },
			{ 200, 200, 200 }
		);

		// should be rendered correctly
		auto const col1 = find_most_red_pixel(surface);
		REQUIRE(200 == int(col1.r));
		REQUIRE(200 == int(col1.g));
		REQUIRE(200 == int(col1.b));

		// Draw over same triangle with different color and vertex order
		draw_triangle_solid(surface,
			{ 60.f, 5.f }, { 77.f, 310.f }, { 300.f, 150.f },
			{ 150, 150, 150 }
		);
		auto const col2 = find_most_red_pixel(surface);
		REQUIRE(150 == int(col2.r));
		REQUIRE(150 == int(col2.g));
		REQUIRE(150 == int(col2.b));

		// Draw over same triangle with different color and vertex order
		draw_triangle_solid(surface,
			 { 77.f, 310.f }, { 300.f, 150.f }, { 60.f, 5.f },
			{ 100, 100, 100 }
		);
		auto const col3 = find_most_red_pixel(surface);
		REQUIRE(100 == int(col3.r));
		REQUIRE(100 == int(col3.g));
		REQUIRE(100 == int(col3.b));
	}

	SECTION("Overlapping interp")
	{
		// Draw over same triangle
		draw_triangle_interp(surface,
			{ 160.f, 255.f }, { 450.f, 100.f }, { 267.f, 360.f },
			{ 200, 200, 200 },
			{ 200, 200, 200 },
			{ 200, 200, 200 }
		);

		// should be rendered correctly
		auto const col11 = find_most_red_pixel(surface);
		REQUIRE(129 == int(col11.r));
		REQUIRE(129 == int(col11.g));
		REQUIRE(129 == int(col11.b));

		// Draw over same triangle with different color and vertex order
		draw_triangle_interp(surface,
			{ 450.f, 100.f }, { 267.f, 360.f }, { 160.f, 255.f },
			{ 150, 150, 150},
			{ 150, 150, 150 },
			{ 150, 150, 150 }
		);
		auto const col22 = find_most_red_pixel(surface);
		REQUIRE(108 == int(col22.r));
		REQUIRE(108 == int(col22.g));
		REQUIRE(108 == int(col22.b));

		// Draw over same triangle with different color and vertex order
		draw_triangle_interp(surface,
			{ 267.f, 360.f }, { 160.f, 255.f }, { 450.f, 100.f },
			{ 255, 0, 0 },
			{ 255, 0, 0 },
			{ 255, 0, 0 }
		);
		auto const col33 = find_most_red_pixel(surface);
		REQUIRE(133 == int(col33.r));
		REQUIRE(0 == int(col33.g));
		REQUIRE(0 == int(col33.b));
	}
}