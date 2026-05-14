#include <catch2/catch_amalgamated.hpp>

#include <algorithm>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE("Intersection lines", "[cull]")
{
	Surface surface(640, 480);
	surface.clear();
	
	// This is added test cases NO.1 
	SECTION("Arrow up")
	{
		draw_line_solid(surface,
			{ 0.f, 0.f }, { 320.f, 480.f },
			{ 255, 0, 0 } // red
		);
		draw_line_solid(surface,
			{ 320.f, 480.f }, { 640.f, 0.f },
			{ 0, 255, 0 } // green
		);

		auto const counts = count_pixel_neighbours(surface);

		// P0 and P2 should have one neighbour each
		REQUIRE(2 == counts[1]);

		// There should be a non-zero number of pixels with two neighbours, as
		// the line is longer than 2 pixels.
		REQUIRE(counts[2] > 0);

		// No pixels should have zero neighbours (=isolated)
		REQUIRE(0 == counts[0]);

		// There should be no pixels with more than two neighbours!
		for (std::size_t i = 3; i < counts.size(); ++i)
			REQUIRE(0 == counts[i]);
	}
	SECTION("Arrow Down")
	{
		draw_line_solid(surface,
			{ 50.f, 480.f }, { 320.f, 0.f },
			{ 0, 0, 255 } // red
		);
		draw_line_solid(surface,
			{ 320.f, 0.f }, { 590.f, 480.f },
			{ 255, 0, 0 } // green
		);

		auto const counts = count_pixel_neighbours(surface);

		// P0 and P2 should have one neighbour each
		REQUIRE(2 == counts[1]);

		// There should be a non-zero number of pixels with two neighbours, as
		// the line is longer than 2 pixels.
		REQUIRE(counts[2] > 0);

		// No pixels should have zero neighbours (=isolated)
		REQUIRE(0 == counts[0]);

		// There should be no pixels with more than two neighbours!
		for (std::size_t i = 3; i < counts.size(); ++i)
			REQUIRE(0 == counts[i]);
	}
	SECTION("Arrow Left")
	{
		draw_line_solid(surface,
			{ 320.f, 480.f }, { 0.f, 240.f },
			{ 255,0,255 }
		);
		draw_line_solid(surface,
			{ 0.f, 240.f }, { 320.f, 0.f },
			{ 0,255,255 }
		);
		// test for vertical and horizontal gap
		REQUIRE(2 == max_row_pixel_count(surface));
		REQUIRE(1 == max_col_pixel_count(surface));

		auto const counts = count_pixel_neighbours(surface);

		// P0 and P2 should have one neighbour each
		REQUIRE(2 == counts[1]);

		// There should be a non-zero number of pixels with two neighbours, as
		// the line is longer than 2 pixels.
		REQUIRE(counts[2] > 0);

		// No pixels should have zero neighbours (=isolated)
		REQUIRE(0 == counts[0]);

		// There should be no pixels with more than two neighbours!
		for (std::size_t i = 3; i < counts.size(); ++i)
			REQUIRE(0 == counts[i]);
	}
	SECTION("Arrow Right")
	{
		draw_line_solid(surface,
			{ 320.f, 480.f }, { 640.f, 240.f },
			{ 255,0,255 }
		);
		draw_line_solid(surface,
			{ 640.f, 240.f }, { 320.f, 0.f },
			{ 0,255,255 }
		);
		auto const counts = count_pixel_neighbours(surface);

		// There should be no pixels with more than two neighbours!
		for (std::size_t i = 3; i < counts.size(); ++i)
			REQUIRE(0 == counts[i]);

	}
	SECTION("Two diagonal line")
	{
		draw_line_solid(surface,
			{ 0.f, 0.f }, { 640.f, 480.f },
			{ 255,0,255 }
		);
		draw_line_solid(surface,
			{ 0.f, 480.f }, { 640.f, 0.f },
			{ 0,255,255 }
		);
		auto const counts = count_pixel_neighbours(surface);

		// No pixels should have zero neighbours (=isolated)
		REQUIRE(0 == counts[0]);

		// test for vertical and horizontal gap
		REQUIRE(2 == max_row_pixel_count(surface));
		REQUIRE(1 == max_col_pixel_count(surface));
	}
}