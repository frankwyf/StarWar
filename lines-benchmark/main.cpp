#include <benchmark/benchmark.h>

#include "../draw2d/draw.hpp"
#include "../draw2d/surface.hpp"

namespace
{

	// Bresenham line drawing implementation used for benchmark comparison
	void draw_line_solid_Bresenham(Surface& aSurface, Vec2f aBegin, Vec2f aEnd, ColorU8_sRGB aColor)
	{
		// handle degenerate lines
		if (aBegin.x == aEnd.x && aBegin.y == aEnd.y) {
			// set the pixel at (aX,aY) to the specified color
			aSurface.set_pixel_srgb(static_cast<int>(aBegin.x), static_cast<int>(aBegin.y), aColor);
			return;
		}

		// Check if the clipped line is entirely outside the window: Culling
		if (aBegin.x >= aSurface.get_width() || aEnd.x < 0 || aBegin.y >= aSurface.get_height() || aEnd.y < 0) {
			return;
		}

		// Clip against x = 0
		if (aBegin.x < 0) {
			// Calculate t value for intersection with x = 0
			double t = -aBegin.x / (aEnd.x - aBegin.x);
			// Calculate the y coordinate of the intersection
			double y = aBegin.y + t * (aEnd.y - aBegin.y);
			// Set the start point to the intersection
			aBegin.x = 0;
			aBegin.y = static_cast<float>(y);
		}

		// Clip against x = windowWidth
		if (aEnd.x >= aSurface.get_width()) {
			// Calculate t value for intersection with x = windowWidth
			double t = (aSurface.get_width() - 1 - aBegin.x) / (aEnd.x - aBegin.x);
			// Calculate the y coordinate of the intersection
			double y = aBegin.y + t * (aEnd.y - aBegin.y);
			// Set the end point to the intersection
			aEnd.x = static_cast<float> (aSurface.get_width()) - 1;
			aEnd.y = static_cast<float>(y);
		}

		// Clip against y = 0
		if (aBegin.y < 0) {
			// Calculate t value for intersection with y = 0
			double t = -aBegin.y / (aEnd.y - aBegin.y);
			// Calculate the x coordinate of the intersection
			double x = aBegin.x + t * (aEnd.x - aBegin.x);
			// Set the start point to the intersection
			aBegin.x = static_cast<float>(x);
			aBegin.y = 0;
		}

		// Clip against y = windowHeight
		if (aEnd.y >= aSurface.get_height()) {
			// Calculate t value for intersection with y = windowHeight
			double t = (aSurface.get_height() - 1 - aBegin.y) / (aEnd.y - aBegin.y);
			// Calculate the x coordinate of the intersection
			double x = aBegin.x + t * (aEnd.x - aBegin.x);
			// Set the end point to the intersection
			aEnd.x = static_cast<float> (x);
			aEnd.y = static_cast<float> (aSurface.get_height()) - 1;
		}

		// Clip against the surface boundaries using Bresenham's line algorithm
		int x0 = static_cast<int>(aBegin.x);
		int y0 = static_cast<int>(aBegin.y);
		int x1 = static_cast<int>(aEnd.x);
		int y1 = static_cast<int>(aEnd.y);

		int dx = std::abs(x1 - x0);
		int dy = std::abs(y1 - y0);
		int sx = x0 < x1 ? 1 : -1;
		int sy = y0 < y1 ? 1 : -1;
		int error = dx - dy;

		while (true) {
			// Set the pixel color if it is inside the surface
			if (x0 >= 0 && static_cast<unsigned int>(x0) < aSurface.get_width() && y0 >= 0 && static_cast<unsigned int>(y0) < aSurface.get_height()) {
				aSurface.set_pixel_srgb(x0, y0, aColor);
			}

			// Check if the line has reached the end point
			if (x0 == x1 && y0 == y1) {
				break;
			}

			int error2 = error * 2;

			if (error2 > -dy) {
				error -= dy;
				x0 += sx;
			}

			if (error2 < dx) {
				error += dx;
				y0 += sy;
			}
		}
	}
}

	void benchamark_Bresenham( benchmark::State& aState )
	{
		auto width = static_cast<std::uint32_t>(aState.range(0));
		auto height = static_cast<std::uint32_t>(aState.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 }; 


		std::vector<std::pair<Vec2f, Vec2f>> lines = {
			{{0.0f, 0.0f}, {static_cast<float>(width - 1), static_cast<float>(height - 1)}}, // diagonal line
			{{0.0f, static_cast<float>(height) / 2.0f}, {static_cast<float>(width - 1), static_cast<float>(height) / 2.0f}}, // horizontal line
			{{static_cast<float>(width) / 2.0f, 0.0f}, {static_cast<float>(width) / 2.0f, static_cast<float>(height - 1)}}, // vertical line
		};

		for (auto _ : aState) {
			surface.clear();
			for (auto& line : lines) {
				aBegin = line.first;
				aEnd = line.second;
				draw_line_solid_Bresenham(surface, aBegin, aEnd, aColor); 
			}
			benchmark::ClobberMemory();
		}
	}

	void benchmark_Bresenham_VerticalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { static_cast<float>(width) / 2.0f, 0.0f };
			aEnd = { static_cast<float>(width) / 2.0f, static_cast<float>(height - 1) };
			draw_line_solid_Bresenham(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}

	void benchmark_Bresenham_HorizontalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { 0.0f, static_cast<float>(height) / 2.0f };
			aEnd = { static_cast<float>(width - 1), static_cast<float>(height) / 2.0f };
			draw_line_solid_Bresenham(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}

	void benchmark_Bresenham_DiagonalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { 0.0f, 0.0f };
			aEnd = { static_cast<float>(width - 1), static_cast<float>(height - 1) };
			draw_line_solid_Bresenham(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}


	void benchamark_DDA(benchmark::State& aState)
	{
		auto width = static_cast<std::uint32_t>(aState.range(0));
		auto height = static_cast<std::uint32_t>(aState.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };


		std::vector<std::pair<Vec2f, Vec2f>> lines = {
			{{0.0f, 0.0f}, {static_cast<float>(width - 1), static_cast<float>(height - 1)}}, 
			{{0.0f, static_cast<float>(height) / 2.0f}, {static_cast<float>(width - 1), static_cast<float>(height) / 2.0f}}, 
			{{static_cast<float>(width) / 2.0f, 0.0f}, {static_cast<float>(width) / 2.0f, static_cast<float>(height - 1)}}, 
		};

		for (auto _ : aState) {
			surface.clear();
			for (auto& line : lines) {
				aBegin = line.first;
				aEnd = line.second;
				draw_line_solid(surface, aBegin, aEnd, aColor); 
			}
			benchmark::ClobberMemory();
		}
	}

	void benchmark_DDA_VerticalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { static_cast<float>(width) / 2.0f, 0.0f };
			aEnd = { static_cast<float>(width) / 2.0f, static_cast<float>(height - 1) };
			draw_line_solid(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}

	void benchmark_DDA_HorizontalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { 0.0f, static_cast<float>(height) / 2.0f };
			aEnd = { static_cast<float>(width - 1), static_cast<float>(height) / 2.0f };
			draw_line_solid(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}

	void benchmark_DDA_DiagonalLine(benchmark::State& state)
	{
		auto width = static_cast<std::uint32_t>(state.range(0));
		auto height = static_cast<std::uint32_t>(state.range(1));
		Surface surface(width, height);
		Vec2f aBegin, aEnd;
		ColorU8_sRGB aColor{ 255, 255, 255 };

		for (auto _ : state) {
			surface.clear();
			aBegin = { 0.0f, 0.0f };
			aEnd = { static_cast<float>(width - 1), static_cast<float>(height - 1) };
			draw_line_solid(surface, aBegin, aEnd, aColor);
			benchmark::ClobberMemory();
		}
	}


BENCHMARK(benchamark_Bresenham)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchamark_DDA)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_Bresenham_VerticalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_Bresenham_HorizontalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_Bresenham_DiagonalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_DDA_VerticalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_DDA_HorizontalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(benchmark_DDA_DiagonalLine)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK_MAIN();
