/* Inline functions need to have a definition that is visible to the compiler
 * whenever the function is used. THey could be define in the header that
 * declares the function. However, to keep the definitions and declarations
 * somewhat apart, it is a common practice to move them to an "inline" file
 * such as this one (*.inl extension). This file is then #include:ed at the end
 * of the header, to ensure that whoever includes the header also automatically
 * includes the inline file.
 *
 * Inlining allows us to avoid any overheads related to call (when building in
 * "release mode" / with optimizations enabled). This makes functions like
 * set_pixel_srgb() zero overhead abstractions.
 *
 */
inline
void Surface::set_pixel_srgb( Index aX, Index aY, ColorU8_sRGB const& aColor )
{
	assert( aX < mWidth && aY < mHeight ); // IMPORTANT! This line must remain the first line in this function!

	//TODO: your implementation goes here
	// set the pixel at (aX,aY) to the specified color
	
	// using the get_linear_index() function to get the linear memory index of the pixel
	int pixelIndex = get_linear_index(aX, aY);

	// fill the pixel with the color
	mSurface[pixelIndex * 4 + 0] = aColor.r;
	mSurface[pixelIndex * 4 + 1] = aColor.g;
	mSurface[pixelIndex * 4 + 2] = aColor.b;

}

inline 
auto Surface::get_width() const noexcept -> Index
{
	return mWidth;
}
inline
auto Surface::get_height() const noexcept -> Index
{
	return mHeight;
}

inline
Surface::Index Surface::get_linear_index( Index aX, Index aY ) const noexcept
{
	//TODO: your implementation goes here
	// return the linear memory index of pixel (aX,aY) by usingt the row major order
	int Linear_index = aX + aY * mWidth;
	return Linear_index;
}
