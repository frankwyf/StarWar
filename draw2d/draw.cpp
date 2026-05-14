#include "draw.hpp"

#include <algorithm>

#include <cmath>

#include "surface.hpp"

void draw_line_solid(Surface& aSurface, Vec2f aBegin, Vec2f aEnd, ColorU8_sRGB aColor)
{
	// handle degenerate lines
	if (aBegin.x == aEnd.x && aBegin.y == aEnd.y) {
		int const x = static_cast<int>(aBegin.x);
		int const y = static_cast<int>(aBegin.y);
		if( x >= 0 && y >= 0 && x < static_cast<int>(aSurface.get_width()) && y < static_cast<int>(aSurface.get_height()) )
			aSurface.set_pixel_srgb(static_cast<Surface::Index>(x), static_cast<Surface::Index>(y), aColor);
		return;
	}

	// Check if the clipped line is entirely outside the window: Culling
	if (aBegin.x >= aSurface.get_width() || aEnd.x < 0 || aBegin.y >= aSurface.get_height() || aEnd.y < 0) {
		return;
	}

	// Clip against x = 0
	if (aBegin.x < 0) {
		double t = -aBegin.x / (aEnd.x - aBegin.x);
		double y = aBegin.y + t * (aEnd.y - aBegin.y);
		aBegin.x = 0;
		aBegin.y = static_cast<float>(y);
	}
	
	// Clip against x = windowWidth
	if (aEnd.x >= aSurface.get_width()) {
		double t = (aSurface.get_width() - 1 - aBegin.x) / (aEnd.x - aBegin.x);
		double y = aBegin.y + t * (aEnd.y - aBegin.y);
		aEnd.x = static_cast<float> (aSurface.get_width()) - 1;
		aEnd.y = static_cast<float>(y);
	}
	
	// Clip against y = 0
	if (aBegin.y < 0) {
		double t = -aBegin.y / (aEnd.y - aBegin.y);
		double x = aBegin.x + t * (aEnd.x - aBegin.x);
		aBegin.x = static_cast<float>(x);
		aBegin.y = 0;
	}

	// Clip against y = windowHeight
	if (aEnd.y >= aSurface.get_height()) {
		double t = (aSurface.get_height() - 1 - aBegin.y) / (aEnd.y - aBegin.y);
		double x = aBegin.x + t * (aEnd.x - aBegin.x);
		aEnd.x = static_cast<float> (x);
		aEnd.y = static_cast<float> (aSurface.get_height()) - 1;
	}

	double x0 = aBegin.x;
	double y0 = aBegin.y;
	double xEnd = aEnd.x;
	double yEnd = aEnd.y;

	double dx = xEnd - x0;
	double dy = yEnd - y0;
	double steps = std::max(std::abs(dx), std::abs(dy));
	double x_increment = dx / steps;
	double y_increment = dy / steps;

	for (double i = 0; i < steps; i++) {
		int xCurrent = static_cast<int>(x0 + 0.5);
		int yCurrent = static_cast<int>(y0 + 0.5);

		if( xCurrent >= 0 && yCurrent >= 0 && xCurrent < static_cast<int>(aSurface.get_width()) && yCurrent < static_cast<int>(aSurface.get_height()) )
			aSurface.set_pixel_srgb(static_cast<Surface::Index>(xCurrent), static_cast<Surface::Index>(yCurrent), aColor);

		x0 += x_increment;
		y0 += y_increment;
	}
}


void draw_triangle_wireframe( Surface& aSurface, Vec2f aP0, Vec2f aP1, Vec2f aP2, ColorU8_sRGB aColor )
{
	// Draw a single triangle using three line segments.
	draw_line_solid(aSurface, aP0, aP1, aColor);
	draw_line_solid(aSurface, aP1, aP2, aColor);
	draw_line_solid(aSurface, aP2, aP0, aColor);
}


void draw_triangle_solid(Surface& aSurface, Vec2f aP0, Vec2f aP1, Vec2f aP2, ColorU8_sRGB aColor)
{
	// handle degenerate triangles,  according to test case degenrate.cpp, they should be ignored
	// 1. same vertex (two or three vertices are the same)
	if ((aP0.x == aP1.x && aP0.y == aP1.y) ||
		(aP0.x == aP2.x && aP0.y == aP2.y) ||
		(aP1.x == aP2.x && aP1.y == aP2.y) ||
		(aP0.x == aP1.x && aP0.x == aP2.x && aP0.y == aP1.y && aP0.y == aP2.y))
		return;

	// 2. consider the three points are on the same line, then they must have the same slope
	if ((aP0.y - aP1.y) / (aP0.x - aP1.x) == (aP0.y - aP2.y) / (aP0.x - aP2.x) 
		|| (aP0.x == aP1.x && aP0.x == aP2.x) || (aP0.x == aP1.x && aP0.y == aP1.y) 
		|| (aP0.x == aP2.x && aP0.y == aP2.y) || (aP1.x == aP2.x && aP1.y == aP2.y)) {
		return;
	}

	// Calculate the function of three edges
	float k01 = (float)(aP1.y - aP0.y) / (float)(aP1.x - aP0.x);
	float k02 = (float)(aP2.y - aP0.y) / (float)(aP2.x - aP0.x);
	float k12 = (float)(aP1.y - aP2.y) / (float)(aP1.x - aP2.x);

	float b01 = (float)aP0.y - k01 * (float)aP0.x;
	float b02 = (float)aP2.y - k02 * (float)aP2.x;
	float b12 = (float)aP1.y - k12 * (float)aP1.x;


	// Loop through the bounding box of the triangle
	for (int x = std::min({ static_cast<int>(aP0.x), static_cast<int>(aP1.x),  static_cast<int>(aP2.x) }); 
		x < std::max({ static_cast<int>(aP0.x), static_cast<int>(aP1.x),  static_cast<int>(aP2.x) }) ; x++)
	{
		for (int y = std::min({ static_cast<int>(aP0.y), static_cast<int>(aP1.y),  static_cast<int>(aP2.y) }); 
			y < std::max({ static_cast<int>(aP0.y), static_cast<int>(aP1.y),  static_cast<int>(aP2.y) }); y++)
		{
			//determine the factors of the barycentric coordinates
			float factor1 = (y - (k12 * x + b12)) * (aP0.y - (k12 * aP0.x + b12));
			float factor2 = (y - (k02 * x + b02)) * (aP1.y - (k02 * aP1.x + b02));
			float factor3 = (y - (k01 * x + b01)) * (aP2.y - (k01 * aP2.x + b01));
			
			// handle division by zero situations, ex. vertical lines
			if (aP2.x == aP1.x) factor1 = (x - aP2.x) * (aP0.x - aP2.x);
			if (aP0.x == aP2.x ) factor2 = (x - aP0.x) * (aP1.x - aP0.x);
			if (aP1.x == aP0.x) factor3 = (x - aP1.x) * (aP2.x - aP1.x);

			// Check if the pixel is inside the triangle and inside the surface
			if (factor1 > 0 && factor2 > 0 && factor3 > 0 &&
				x >= 0 && static_cast<unsigned int>(x) < aSurface.get_width() && y >= 0
				&& static_cast<unsigned int>(y) < aSurface.get_height())
			{
				aSurface.set_pixel_srgb(x, y, aColor);
			}
		}
	}
	// for vertcial lines
	if (aP0.x == aP1.x || aP0.y == aP1.y || k01 == 1.f) draw_line_solid(aSurface, aP0, aP1, aColor);
	if (aP0.x == aP2.x || aP0.y == aP2.y || k02 == 1.f) draw_line_solid(aSurface, aP0, aP2, aColor);
	if (aP1.x == aP2.x || aP1.y == aP2.y || k12 == 1.f) draw_line_solid(aSurface, aP1, aP2, aColor);

	// set the three vertices to the specified colors if they are inside the surface
	if (aP0.x > 0 && aP0.x < aSurface.get_width() && aP0.y > 0 && aP0.y < aSurface.get_height() &&
		aP1.x > 0 && aP1.x < aSurface.get_width() && aP1.y > 0 && aP1.y < aSurface.get_height() &&
		aP2.x > 0 && aP2.x < aSurface.get_width() && aP2.y > 0 && aP2.y < aSurface.get_height()) {
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP0.x), static_cast<Surface::Index>(aP0.y), aColor);
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP1.x), static_cast<Surface::Index>(aP1.y), aColor);
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP2.x), static_cast<Surface::Index>(aP2.y), aColor);
	}
}

void draw_triangle_interp(Surface& aSurface, Vec2f aP0, Vec2f aP1, Vec2f aP2, ColorF aC0, ColorF aC1, ColorF aC2)
{
	//TODO: your implementation goes here
	// This function draws a single triangle defined by its three vertices(aP0, aP1 and aP2).
	// The color of each vertex is specified by the corresponding aC0, aC1 and aC2 arguments.
	// These colors should be interpolated across the triangle with barycentric interpolation.

	// step 1: iterate over the pixels in the bounding box
	// Loop through the bounding box of the triangle
	for (int x = std::min({ static_cast<int>(aP0.x), static_cast<int>(aP1.x),  static_cast<int>(aP2.x) });
		x < std::max({ static_cast<int>(aP0.x), static_cast<int>(aP1.x),  static_cast<int>(aP2.x) }); x++)
	{
		for (int y = std::min({ static_cast<int>(aP0.y), static_cast<int>(aP1.y),  static_cast<int>(aP2.y) });
			y < std::max({ static_cast<int>(aP0.y), static_cast<int>(aP1.y),  static_cast<int>(aP2.y) }); y++)
		{
			// step 2: calculate the barycentric coordinates of the pixel accroding to the formula
			float alpha = (- (x - aP1.x) * (aP2.y - aP1.y) + (y - aP1.y) * (aP2.x - aP1.x)) / (- (aP0.x - aP1.x) * (aP2.y - aP1.y) + (aP0.y - aP1.y) * (aP2.x - aP1.x));
			float beta = (-(x - aP2.x) * (aP0.y - aP2.y) + (y - aP2.y) * (aP0.x - aP2.x)) / (-(aP1.x - aP2.x) * (aP0.y - aP2.y) + (aP1.y - aP2.y) * (aP0.x - aP2.x));
			float gamma = 1.0f - alpha - beta;

			// step 3: check if the pixel is inside the triangle
			if (alpha > 0 && beta > 0 && gamma > 0)
			{
				// step 4: calculate the interpolated color of the pixel
				ColorF interpolatedColor = {
					alpha * aC0.r + beta * aC1.r + gamma * aC2.r,
					alpha * aC0.g + beta * aC1.g + gamma * aC2.g,
					alpha * aC0.b + beta * aC1.b + gamma * aC2.b
				};

				// Convert the interpolated color to sRGB
				ColorU8_sRGB interpolatedColor_sRGB = linear_to_srgb(interpolatedColor);

				// step 5: set the pixel to the interpolated color if it is inside the surface
				if (x >= 0 && static_cast<unsigned int>(x) < aSurface.get_width() && y >= 0 && static_cast<unsigned int>(y) < aSurface.get_height())
				aSurface.set_pixel_srgb(x, y, interpolatedColor_sRGB);
			}
		}
	}
	// set the three vertices to the specified colors if they are inside the surface
	if (aP0.x > 0 && aP0.x < aSurface.get_width() && aP0.y > 0 && aP0.y < aSurface.get_height() &&
		aP1.x > 0 && aP1.x < aSurface.get_width() && aP1.y > 0 && aP1.y < aSurface.get_height() &&
		aP2.x > 0 && aP2.x < aSurface.get_width() && aP2.y > 0 && aP2.y < aSurface.get_height()) {
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP0.x), static_cast<Surface::Index>(aP0.y), linear_to_srgb( aC0));
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP1.x), static_cast<Surface::Index>(aP1.y), linear_to_srgb(aC1));
		aSurface.set_pixel_srgb(static_cast<Surface::Index>(aP2.x), static_cast<Surface::Index>(aP2.y), linear_to_srgb(aC2));
	}
}


void draw_rectangle_solid( Surface& aSurface, Vec2f aMinCorner, Vec2f aMaxCorner, ColorU8_sRGB aColor )
{
	int x0 = static_cast<int>(std::floor(aMinCorner.x));
	int y0 = static_cast<int>(std::floor(aMinCorner.y));
	int x1 = static_cast<int>(std::ceil(aMaxCorner.x));
	int y1 = static_cast<int>(std::ceil(aMaxCorner.y));

	int const w = static_cast<int>(aSurface.get_width());
	int const h = static_cast<int>(aSurface.get_height());

	x0 = std::max( 0, x0 );
	y0 = std::max( 0, y0 );
	x1 = std::min( w, x1 );
	y1 = std::min( h, y1 );

	if( x0 >= x1 || y0 >= y1 )
		return;

	for( int i = x0; i < x1; ++i )
	{
		for( int j = y0; j < y1; ++j )
		{
			aSurface.set_pixel_srgb( i, j, aColor );
		}
	}
}

void draw_rectangle_outline( Surface& aSurface, Vec2f aMinCorner, Vec2f aMaxCorner, ColorU8_sRGB aColor )
{
	int x0 = static_cast<int>(std::floor(aMinCorner.x));
	int y0 = static_cast<int>(std::floor(aMinCorner.y));
	int x1 = static_cast<int>(std::ceil(aMaxCorner.x));
	int y1 = static_cast<int>(std::ceil(aMaxCorner.y));

	int const w = static_cast<int>(aSurface.get_width());
	int const h = static_cast<int>(aSurface.get_height());

	x0 = std::max( 0, x0 );
	y0 = std::max( 0, y0 );
	x1 = std::min( w, x1 );
	y1 = std::min( h, y1 );

	if( x0 >= x1 || y0 >= y1 )
		return;

	for( int i = x0; i < x1; ++i )
	{
		for( int j = y0; j < y1; ++j )
		{
			if( i == x0 || i == x1 - 1 || j == y0 || j == y1 - 1 )
				aSurface.set_pixel_srgb( i, j, aColor );
		}
	}
}
