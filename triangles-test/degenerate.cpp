#include <catch2/catch_amalgamated.hpp>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE( "Degenerate", "[special][!mayfail]" )
{
	// Degenerate triangles are triangles with zero area. A simple exaple of a
	// degenerate triangle is one where two of the vertices coincide. 
	//
	// Most graphics APIs will discard degenerate triangles with the argument 
	// that they are zero-area and thus cannot cover any pixels. Some 
	// techniques even rely on this fact (triangle strips).
	//
	// However, handling of these cases can vary between rasterizers.
	// Consequently, this test case is marked as [!mayfail].

	Surface surface( 320, 240 );
	surface.clear();

	SECTION( "same vertex" )
	{
		draw_triangle_solid( surface,
			{ 21.f, 29.f }, { 221.f, 129.f }, { 21.f, 29.f },
			{ 100, 100, 100 }
		);

		auto const col = find_most_red_pixel( surface );
		REQUIRE( 0 == int(col.r) );
		REQUIRE( 0 == int(col.g) );
		REQUIRE( 0 == int(col.b) );
	}

	// three points are the same line
	SECTION("same vertex (three)")
	{
		draw_triangle_solid(surface,
			{ 21.f, 29.f }, { 21.f, 29.f }, { 21.f, 29.f },
			{ 100, 100, 100 }
		);

		auto const col = find_most_red_pixel(surface);
		REQUIRE(0 == int(col.r));
		REQUIRE(0 == int(col.g));
		REQUIRE(0 == int(col.b));
	}

	SECTION( "midpoint" )
	{
		// Note that this gets very tricky with diagonal "triangles", since
		// they will suffer more from rounding problems.
		//
		// This "triangle" is axis aligned.
		draw_triangle_solid( surface,
			{ 120.f, 120.f }, { 200.f, 120.f }, { 160.f, 120.f },
			{ 100, 100, 100 }
		);

		auto const col = find_most_red_pixel( surface );
		REQUIRE( 0 == int(col.r) );
		REQUIRE( 0 == int(col.g) );
		REQUIRE( 0 == int(col.b) );
	}
}
