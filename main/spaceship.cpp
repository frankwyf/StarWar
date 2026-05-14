#include "spaceship.hpp"

#include <cstdio>

#include "../draw2d/shape.hpp"

/* Custom spaceship shape configuration
 *
 * - SPACESHIP_DEFAULT: built-in ship shape
 * - SPACESHIP_CUSTOM : alternative custom shape defined below
 */

#define SPACESHIP_DEFAULT 1
#define SPACESHIP_CUSTOM 2

#ifndef SPACESHIP
#	define SPACESHIP SPACESHIP_DEFAULT
#endif

LineStrip make_spaceship_shape()
{
#	if SPACESHIP == SPACESHIP_DEFAULT
	static constexpr float xs[] = { 250.f, 200.f, 150.f, 100.f, 000.f, 040.f, -50.f, -140.f, -170.f };
	static constexpr float ys[] = { 190.f, 180.f, 70.f, 50.f, 30.f, 20.f };

	LineStrip spaceship{ { 
		{ 0.2f * xs[0], 0.2f * +ys[5] }, // upper half. starts at front, goes towards the back
		{ 0.2f * xs[1], 0.2f * +ys[3] },
		{ 0.2f * xs[2], 0.2f * +ys[3] },
		{ 0.2f * xs[3], 0.2f * +ys[4] }, 
		{ 0.2f * xs[4], 0.2f * +ys[4] },
		{ 0.2f * xs[4], 0.2f * +ys[2] },
		{ 0.2f * xs[5], 0.2f * +ys[1] },
		{ 0.2f * xs[6], 0.2f * +ys[0] },
		{ 0.2f * xs[8], 0.2f * +ys[2] },
		{ 0.2f * xs[7], 0.2f * +ys[3] },

		{ 0.2f * xs[7], 0.2f * -ys[3] }, // lower half, starts at the back and goes towards the front
		{ 0.2f * xs[8], 0.2f * -ys[2] }, // this is essentially the same as the upper half, except in reverse.
		{ 0.2f * xs[6], 0.2f * -ys[0] },
		{ 0.2f * xs[5], 0.2f * -ys[1] },
		{ 0.2f * xs[4], 0.2f * -ys[2] },
		{ 0.2f * xs[4], 0.2f * -ys[4] },
		{ 0.2f * xs[3], 0.2f * -ys[4] }, 
		{ 0.2f * xs[2], 0.2f * -ys[3] },
		{ 0.2f * xs[1], 0.2f * -ys[3] },
		{ 0.2f * xs[0], 0.2f * -ys[5] }, 

		{ 0.2f * xs[0], 0.2f * +ys[5] } // link back to beginning (connects both sides at the "front")
	} };
#	elif SPACESHIP == SPACESHIP_CUSTOM

	// TODO: YOUR DESIGN GOES HERE
	static constexpr float xs[] = { 0.f, -50.f, -100.f, -150.f, -200.f, -250.f, -550.f, -350.f, -400.f };
	static constexpr float ys[] = { 0.f, 50.f, 100.f, 150.f, 200.f, 250.f };

	LineStrip spaceship{ {
	{0.2f * xs[0], 0.2f * +ys[0]}, // lower right wing start
	{0.2f * xs[1], 0.2f * +ys[1]}, // lower right wing curve
	{0.2f * xs[2], 0.2f * +ys[3]}, // lower right wing tip
	{0.2f * xs[3], 0.2f * +ys[2]}, // lower right wing inner
	{0.2f * xs[4], 0.2f * +ys[1]}, // lower right wing end
	{0.2f * xs[5], 0.2f * +ys[1]}, // lower body start
	{0.2f * xs[6], 0.2f * +ys[2]}, // lower body end
	{0.2f * xs[7], 0.2f * +ys[3]}, // lower left wing start
	{0.2f * xs[8], 0.2f * +ys[5]}, // lower left wing tip
	{0.2f * xs[7], 0.2f * +ys[4]}, // lower left wing curve
	{0.2f * xs[6], 0.2f * +ys[3]}, // lower left wing end
	{0.2f * xs[6], 0.2f * -ys[3]}, // upper left wing start
	{0.2f * xs[7], 0.2f * -ys[4]}, // upper left wing curve
	{0.2f * xs[8], 0.2f * -ys[5]}, // upper left wing tip
	{0.2f * xs[7], 0.2f * -ys[3]}, // upper left wing end
	{0.2f * xs[6], 0.2f * -ys[2]}, // upper body start
	{0.2f * xs[5], 0.2f * -ys[1]}, // upper body end
	{0.2f * xs[4], 0.2f * -ys[1]}, // upper right wing start
	{0.2f * xs[3], 0.2f * -ys[2]}, // upper right wing inner
	{0.2f * xs[2], 0.2f * -ys[3]}, // upper right wing tip
	{0.2f * xs[1], 0.2f * -ys[1]}, // upper right wing curve
	{0.2f * xs[0], 0.2f * -ys[0]}  // upper right wing end
} };

#	endif
	if( spaceship.vertex_count() > 32 )
	{
		std::fprintf( stderr, "WARNING: you must use at most 32 points for your custom spaceship design. You are currently using %zu\n", spaceship.vertex_count() );
	}

	return spaceship;
}
