inline
ColorU8_sRGB_Alpha ImageRGBA::get_pixel( Index aX, Index aY ) const
{
	assert( aX < mWidth && aY < mHeight ); // Leave this at the top of the function.

	//TODO: your implementation goes here

	// get the linear memory index of the pixel
	int index = get_linear_index(aX, aY);

	ColorU8_sRGB_Alpha pixelColor;

	// get the RGB color  and alpha value of the pixel
	pixelColor.r = mData[index * 4 + 0];
	pixelColor.g = mData[index * 4 + 1];
	pixelColor.b = mData[index * 4 + 2];
	pixelColor.a = mData[index * 4 + 3]; // alpha channel

	return pixelColor;
}

inline
auto ImageRGBA::get_width() const noexcept -> Index
{
	return mWidth;
}
inline
auto ImageRGBA::get_height() const noexcept -> Index
{
	return mHeight;
}

inline
std::uint8_t* ImageRGBA::get_image_ptr() noexcept
{
	return mData;
}
inline
std::uint8_t const* ImageRGBA::get_image_ptr() const noexcept
{
	return mData;
}

inline
ImageRGBA::Index ImageRGBA::get_linear_index( Index aX, Index aY ) const noexcept
{
	//TODO: your implementation goes here
	return aY * mWidth + aX;
}
