#include <benchmark/benchmark.h>

#include <algorithm>

#include <cassert>

#include <cstring>

#include "../draw2d/image.hpp"
#include "../draw2d/surface.hpp"

namespace
{
	// Task 1.9.1 : Implement the default blit function for benchmarking using a copy by line approach 
	void blit_masked_byline(Surface& aSurface, ImageRGBA const& aImage, Vec2f aPosition)
	{
		// Loop through the source image lines
		for (int y = 0; static_cast<unsigned int>(y) < aImage.get_height(); y++)
		{

			// Check if the entire line is outside the surface
			if (static_cast<int>(aPosition.y) + y >= 0 && static_cast<unsigned int>(static_cast<int>(aPosition.y) + y) < aSurface.get_height())
			{
				// Calculate the number of pixels to copy in this line
				int pixelsLine = std::min(static_cast<int>(aImage.get_width()), static_cast<int>(aSurface.get_width()) - static_cast<int>(aPosition.x));

				// Check if there are pixels to copy and if the starting point is within the surface
				if (pixelsLine > 0 && static_cast<int>(aPosition.x) < static_cast<int>(aSurface.get_width()))
				{
					// Clip pixels to copy if it goes beyond the right edge of the surface
					pixelsLine = std::min(pixelsLine, static_cast<int>(aSurface.get_width()) - static_cast<int>(aPosition.x));

					// Copy the entire line of pixels from source to destination
					for (int x = 0; x < pixelsLine; x++)
					{
						// Get the source pixel
						ColorU8_sRGB_Alpha srcPixel = aImage.get_pixel(x, y);

						// Check if the destination pixel is within the surface
						if (static_cast<int>(aPosition.x) + x >= 0 && static_cast<unsigned int>(static_cast<int>(aPosition.x) + x) < aSurface.get_width())
						{
							// Update the destination pixel with the source pixel's color
							aSurface.set_pixel_srgb(static_cast<int>(aPosition.x) + x, static_cast<int>(aPosition.y) + y, { srcPixel.r, srcPixel.g, srcPixel.b });
						}
					}
				}
			}
		}
	}

	// Task 1.9.2: Implement the byline blit function for benchmarking using memcpy
	void blit_masked_memcpy(Surface& aSurface, ImageRGBA const& aImage, Vec2f aPosition)
	{
		int copyLength = 0;

		// strating point of the image and surface
		int start = 0;

		if (aPosition.x >= 0 && aPosition.x < aSurface.get_width()) {
			if (aSurface.get_width() - aPosition.x >= aImage.get_width()) {
				copyLength = aImage.get_width();
			}
			else {
				copyLength = static_cast<int> (aSurface.get_width() - aPosition.x);
			}
			start = static_cast<int> (aPosition.x);
		}
		else if (aPosition.x < 0 && aPosition.x + aImage.get_width()>0) {
			copyLength = static_cast<int> (aImage.get_width() + aPosition.x);
			start = 0;
		}

		if (copyLength > 0) {
			for (int y = 0; y < static_cast<int> (aImage.get_height()); ++y) {
				int ys = static_cast<int> (aPosition.y + y);
				if (ys >= 0 && ys < static_cast<int> (aSurface.get_height())) {
					const uint8_t* from = aImage.get_image_ptr() + (y * aImage.get_width() + start) * sizeof(ColorU8_sRGB_Alpha);
					uint8_t* to = const_cast<std::uint8_t*>(aSurface.get_surface_ptr()) + (ys * aSurface.get_width() + static_cast<int>(aPosition.x)) * sizeof(ColorU8_sRGB_Alpha);
					std::size_t s = sizeof(ColorU8_sRGB_Alpha) * copyLength;
					std::memcpy(to, from, s);
				}
			}
		}
	}


	void default_blit_earth_( benchmark::State& aState )
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface( width, height );
		surface.clear();

		auto source = load_image( "assets/earth.png" );
		assert( source );

		for( auto _ : aState )
		{
			blit_masked( surface, *source, {0.f, 0.f} );

			// ClobberMemory() ensures that the compiler won't optimize away
			// our blit operation. (Unlikely, but technically poossible.)
			benchmark::ClobberMemory(); 
		}

		// The following enables the benchmarking library to print information
		// about the memory bandwidth. The total number of bytes processed is
		// *approximatively* two times the total number of bytes in the blit,
		// accounding for both reading and writing. ("Approximatively" since
		// not all pixels are written.)
		auto const maxBlitX = std::min( width, source->get_width() );
		auto const maxBlitY = std::min( height, source->get_height() );

		aState.SetBytesProcessed( 2*maxBlitX*maxBlitY*4 * aState.iterations() );
	}

	void default_blit_earth_small(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/small.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void default_blit_earth_medium(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/medium.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void default_blit_earth_large(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/large.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void byline_blit_earth_(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/earth.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_byline(surface, *source, { 0.f, 0.f });

			// ClobberMemory() ensures that the compiler won't optimize away
			// our blit operation. (Unlikely, but technically poossible.)
			benchmark::ClobberMemory();
		}

		// The following enables the benchmarking library to print information
		// about the memory bandwidth. The total number of bytes processed is
		// *approximatively* two times the total number of bytes in the blit,
		// accounding for both reading and writing. ("Approximatively" since
		// not all pixels are written.)
		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}

	void byline_blit_earth_small(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/small.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_byline(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void byline_blit_earth_medium(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/medium.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_byline(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void byline_blit_earth_large(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/large.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_byline(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}

	void memcpy_blit_earth_(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/earth.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_memcpy(surface, *source, { 0.f, 0.f });

			// ClobberMemory() ensures that the compiler won't optimize away
			// our blit operation. (Unlikely, but technically poossible.)
			benchmark::ClobberMemory();
		}

		// The following enables the benchmarking library to print information
		// about the memory bandwidth. The total number of bytes processed is
		// *approximatively* two times the total number of bytes in the blit,
		// accounding for both reading and writing. ("Approximatively" since
		// not all pixels are written.)
		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void memcpy_blit_earth_small(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/small.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_memcpy(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void memcpy_blit_earth_medium(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/medium.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_memcpy(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}


	void memcpy_blit_earth_large(benchmark::State& aState)
	{
		auto const width = std::uint32_t(aState.range(0));
		auto const height = std::uint32_t(aState.range(1));

		Surface surface(width, height);
		surface.clear();

		auto source = load_image("assets/large.png");
		assert(source);

		for (auto _ : aState)
		{
			blit_masked_memcpy(surface, *source, { 0.f, 0.f });

			benchmark::ClobberMemory();
		}

		auto const maxBlitX = std::min(width, source->get_width());
		auto const maxBlitY = std::min(height, source->get_height());

		aState.SetBytesProcessed(2 * maxBlitX * maxBlitY * 4 * aState.iterations());
	}
}

BENCHMARK(default_blit_earth_small)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Default size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(default_blit_earth_medium)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Default size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(default_blit_earth_)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Default size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK( default_blit_earth_large)
	->Args({ 320, 240 })   // Smaller framebuffer
	->Args({ 1280, 720 })  // Default size
	->Args({ 1920, 1080 }) // Full HD
	->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(byline_blit_earth_small)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(byline_blit_earth_medium)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(byline_blit_earth_)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(byline_blit_earth_large)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;


BENCHMARK(memcpy_blit_earth_small)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(memcpy_blit_earth_medium)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(memcpy_blit_earth_)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK(memcpy_blit_earth_large)
->Args({ 320, 240 })   // Smaller framebuffer
->Args({ 1280, 720 })  // Defaul size
->Args({ 1920, 1080 }) // Full HD
->Args({ 7680, 4320 }) // 8k framebuffer
;

BENCHMARK_MAIN();
