#ifndef MAT22_HPP_1F974C02_D0D1_4FBD_B5EE_A69C88112088
#define MAT22_HPP_1F974C02_D0D1_4FBD_B5EE_A69C88112088

#include <cmath>

#include "vec2.hpp"

/** Mat22f : 2x2 matrix with floats
 *
 * See comments for Vec2f for some discussion.
 *
 * The matrix is stored in row-major order.
 *
 * Example:
 *   Mat22f identity{ 
 *     1.f, 0.f,
 *     0.f, 1.f
 *   };
 */
struct Mat22f
{
	float _00, _01;
	float _10, _11;
};

// Common operators for Mat22f.
// Note that you will need to implement these yourself.

constexpr
Mat22f operator*( Mat22f const& aLeft, Mat22f const& aRight ) noexcept
{
	//TODO: your implementation goes here
	// Implementations of Matrix-matrix multiplication
	return Mat22f{
		aLeft._00 * aRight._00 + aLeft._01 * aRight._10, // 1st row times 1st column
		aLeft._00 * aRight._01 + aLeft._01 * aRight._11, // 1st row times 2nd column
		aLeft._10 * aRight._00 + aLeft._11 * aRight._10, // 2nd row times 1st column
		aLeft._10 * aRight._01 + aLeft._11 * aRight._11 // 2nd row times 2nd column
	};
}

constexpr
Vec2f operator*( Mat22f const& aLeft, Vec2f const& aRight ) noexcept
{
	//TODO: your implementation goes here
	// Implementations of Matrix-vector multiplication
	return Vec2f{
		 aLeft._00 * aRight.x + aLeft._01 * aRight.y,// 1st row of matrix times vector
		 aLeft._10 * aRight.x + aLeft._11 * aRight.y // 2nd row of matrix times vector
	};
}

// Functions:

inline
Mat22f make_rotation_2d( float aAngle ) noexcept
{
	//TODO: your implementation goes here
	// Implementations of Creation of a rotation matrix
	// the formula of 2D  rotation matrix is:
	//							cos(a) -sin(a)
	//							sin(a)  cos(a)
	Mat22f result;

	result._00 = std::cos(aAngle); // element at (0,0)
	result._01 = -std::sin(aAngle); // element at (0,1)
	result._10 = std::sin(aAngle); // element at (1,0)
	result._11 = std::cos(aAngle); // element at (1,1)

	return result;
}

#endif // MAT22_HPP_1F974C02_D0D1_4FBD_B5EE_A69C88112088
