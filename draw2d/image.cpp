#include "image.hpp"

#include <memory>
#include <algorithm>

#include <cstdio>
#include <cstring>
#include <cassert>

#include <stb_image.h>

#include "surface.hpp"

#include "../support/error.hpp"

namespace
{
	struct STBImageRGBA_ : public ImageRGBA
	{
		STBImageRGBA_( Index, Index, std::uint8_t* );
		virtual ~STBImageRGBA_();
	};
}

ImageRGBA::ImageRGBA()
	: mWidth( 0 )
	, mHeight( 0 )
	, mData( nullptr )
{}

ImageRGBA::~ImageRGBA() = default;


std::unique_ptr<ImageRGBA> load_image( char const* aPath )
{
	assert( aPath );

	stbi_set_flip_vertically_on_load( true );

	int w, h, channels;
	stbi_uc* ptr = stbi_load( aPath, &w, &h, &channels, 4 );
	if( !ptr )
		throw Error( "Unable to load image \"%s\"", aPath );

	return std::make_unique<STBImageRGBA_>(
		ImageRGBA::Index(w),
		ImageRGBA::Index(h),
		ptr
	);
}


void blit_masked(Surface& aSurface, ImageRGBA const& aImage, Vec2f aPosition)
{
	//TODO: your implementation goes here

	//loop through the image
	for (int i = aPosition.x; i < aPosition.x + aImage.get_width(); i++) {
		for (int j = aPosition.y; j < aImage.get_height() + aPosition.y; j++) {

			// check if the alpha value is greater than 128 and within the surface
			if (i >= 0 && i < static_cast<int> (aSurface.get_width()) && j >= 0 && j < static_cast<int> (aSurface.get_height())
				&& aImage.get_pixel(i - aPosition.x, j - aPosition.y).a >= 128) {

				// copy the pixel from the image to the surface by calling set_pixel_srgb
				aSurface.set_pixel_srgb(i, j, { aImage.get_pixel(i - aPosition.x, j - aPosition.y).r, 
				aImage.get_pixel(i - aPosition.x, j - aPosition.y).g, aImage.get_pixel(i - aPosition.x, j - aPosition.y).b });
			}
		}
	}
}
 

namespace
{
	STBImageRGBA_::STBImageRGBA_( Index aWidth, Index aHeight, std::uint8_t* aPtr )
	{
		mWidth = aWidth;
		mHeight = aHeight;
		mData = aPtr;
	}

	STBImageRGBA_::~STBImageRGBA_()
	{
		if( mData )
			stbi_image_free( mData );
	}
}
