#include "asteroid_field.hpp"

#include <random>
#include <algorithm>

#include <cmath>
#include <cassert>

#include "../draw2d/shape.hpp"

#include "asteroid.hpp"

namespace
{
	// C++20 adds a number of standardized mathematical constants:
	// https://en.cppreference.com/w/cpp/numeric/constants
	// This defines a custom (worse) one:
	constexpr float kPI = 3.1415926535897932385f; // pi
}

AsteroidField::AsteroidField( RNG& aRNG, std::uint32_t aWidth, std::uint32_t aHeight, float aDensity, float aInitialSpeedStddev, float aMaximumSpeed, float aInitialRotStddev, float aPadding )
	: mInitialSpeed( aInitialSpeedStddev )
	, mMaximumSpeed( aMaximumSpeed )
	, mInitialRot( aInitialRotStddev )
	, mPadding( aPadding )
	, mDensity( aDensity )
	, mRNG( aRNG )
{
	// Compute area of simulation
	mExactExtent = Vec2f{ float(aWidth), float(aHeight) };

	mBoundsMin = Vec2f{ -mPadding, -mPadding };
	mBoundsMax = mExactExtent + Vec2f{ mPadding, mPadding };

	mActualExtent = mBoundsMax - mBoundsMin;

	// Generate initial asteroids
	float const numAsteroidsf = mActualExtent.x*mActualExtent.y * mDensity;
	std::size_t const numAsteroids = std::size_t(numAsteroidsf+0.5f);
	mMaxAsteroids = std::max<std::size_t>( numAsteroids, 1 ) * 3;
	
	mAsteroids.resize( numAsteroids );
	mShapes.reserve( mMaxAsteroids );

	using Uniform_ = std::uniform_real_distribution<float>;
	using Normal_ = std::normal_distribution<float>;

	Uniform_ xpos{ mBoundsMin.x, mBoundsMax.x };
	Uniform_ ypos{ mBoundsMin.y, mBoundsMax.y };

	Uniform_ angle( 0.f, 2*kPI );

	Normal_ vvel{ 0.f, mInitialSpeed };
	Normal_ rots{ 0.f, mInitialRot };

	for( std::size_t i = 0; i < numAsteroids; ++i )
	{
		auto& astr = mAsteroids[i];
		astr.pos = Vec2f{ xpos( mRNG ), ypos( mRNG ) };
		astr.vel = Vec2f{ vvel( mRNG ), vvel( mRNG ) };

		astr.rot = make_rotation_2d( angle( mRNG ) );
		astr.radpersec = rots( mRNG );
		astr.collisionRadius = 40.f;
		astr.small = false;

		// Don't break the speed limits. The space police will get you!
		astr.vel.x = std::clamp( astr.vel.x, -mMaximumSpeed, +mMaximumSpeed );
		astr.vel.y = std::clamp( astr.vel.y, -mMaximumSpeed, +mMaximumSpeed );

		// Create shape
		mShapes.emplace_back( make_asteroid( mRNG ) );
	}
}

AsteroidField::~AsteroidField() = default;


void AsteroidField::update( float aElapsed, Vec2f const& aTransl )
{
	auto const numAsteroids = mAsteroids.size();
	assert( numAsteroids == mShapes.size() );

	for( std::size_t i = 0; i < numAsteroids; ++i )
	{
		auto& astr = mAsteroids[i];
		astr.pos += astr.vel * aElapsed - aTransl;

		if( astr.pos.x < mBoundsMin.x || astr.pos.x > mBoundsMax.x || astr.pos.y < mBoundsMin.y || astr.pos.y > mBoundsMax.x )
		{
			respawn_asteroid_( i );
		}
		else
		{
			astr.rot = make_rotation_2d( astr.radpersec * aElapsed ) * astr.rot;
		}
	}
}

void AsteroidField::draw( Surface& aSurface ) const
{
	auto const numAsteroids = mAsteroids.size();
	assert( numAsteroids == mShapes.size() );

	for( std::size_t i = 0; i < numAsteroids; ++i )
	{
		auto const& astr = mAsteroids[i];
		auto const& shape = mShapes[i];

		shape.draw(
			aSurface,
			astr.rot,
			astr.pos
		);
	}
}

void AsteroidField::resize( std::uint32_t aWidth, std::uint32_t aHeight )
{
	// WARNING: This is a bit of a hack...

	auto const oldMax = mBoundsMax;

	// New area and asteroid count
	mExactExtent = Vec2f{ float(aWidth), float(aHeight) };

	mBoundsMin = Vec2f{ -mPadding, -mPadding };
	mBoundsMax = mExactExtent + Vec2f{ mPadding, mPadding };

	mActualExtent = mBoundsMax - mBoundsMin;

	float const numAsteroidsf = mActualExtent.x*mActualExtent.y * mDensity;
	std::size_t const numAsteroids = std::size_t(numAsteroidsf+0.5f);
	mMaxAsteroids = std::max<std::size_t>( numAsteroids, 1 ) * 3;

	// Remove asteroids now outside
	std::size_t activeAsteroids = 0;

	auto jt = mShapes.begin();
	for( auto it = mAsteroids.begin(); it != mAsteroids.end(); )
	{
		auto const& aster = *it;

		if( aster.pos.x > mBoundsMax.x || aster.pos.y > mBoundsMax.y )
		{
			it = mAsteroids.erase( it );
			jt = mShapes.erase( jt );
		}
		else
		{
			++it;
			++jt;
			++activeAsteroids;
		}
	}

	mAsteroids.resize( numAsteroids );
	activeAsteroids = std::min( activeAsteroids, numAsteroids );

	mShapes.erase( mShapes.begin()+activeAsteroids, mShapes.end() );
	mShapes.reserve( mMaxAsteroids );

	assert( mShapes.size() == activeAsteroids );

	// Generate new asteroids.
	using Normal_ = std::normal_distribution<float>;
	using Uniform_ = std::uniform_real_distribution<float>;

	if( activeAsteroids < numAsteroids )
	{
		auto dd = mBoundsMax - oldMax;
		if( dd.x < 0.f ) dd.x = 0.f;
		if( dd.y < 0.f ) dd.y = 0.f;

		float xarea = dd.x * mBoundsMax.y;
		float yarea = dd.y * (mBoundsMax.x - dd.x);

		Uniform_ area( 0.f, xarea+yarea );
		Uniform_ xax( oldMax.x, oldMax.x+dd.x );
		Uniform_ xay( 0.f, mBoundsMax.y );
		Uniform_ yax( 0.f, mBoundsMax.x - dd.x );
		Uniform_ yay( oldMax.y, oldMax.y+dd.y );

		Uniform_ angle( 0.f, 2*kPI );

		Normal_ vvel{ 0.f, mInitialSpeed };
		Normal_ rots{ 0.f, mInitialRot };

		for( std::size_t i = activeAsteroids; i < numAsteroids; ++i )
		{
			Vec2f pos;

			auto const where = area(mRNG);
			if( where <= xarea )
			{
				pos.x = xax( mRNG );
				pos.y = xay( mRNG );
			}
			else
			{
				pos.x = yax( mRNG );
				pos.y = yay( mRNG );
			}

			auto& astr = mAsteroids[i];

			astr.pos = pos;
			astr.vel = Vec2f{ vvel( mRNG ), vvel( mRNG ) };

			astr.rot = make_rotation_2d( angle( mRNG ) );
			astr.radpersec = rots( mRNG );
			astr.collisionRadius = 40.f;
			astr.small = false;

			astr.vel.x = std::clamp( astr.vel.x, -mMaximumSpeed, +mMaximumSpeed );
			astr.vel.y = std::clamp( astr.vel.y, -mMaximumSpeed, +mMaximumSpeed );

			assert( i == mShapes.size() );
			mShapes.emplace_back( make_asteroid( mRNG ) );
		}
	}

	assert( mAsteroids.size() == mShapes.size() );
}

std::size_t AsteroidField::hit_test_and_destroy( Vec2f const& aPos, float aRadius )
{
	std::size_t hits = 0;

	for( std::size_t i = 0; i < mAsteroids.size(); ++i )
	{
		auto const delta = mAsteroids[i].pos - aPos;
		auto const hitRadius = aRadius + mAsteroids[i].collisionRadius;
		auto const distSq = dot( delta, delta );

		if( distSq <= hitRadius * hitRadius )
		{
			auto const wasSmall = mAsteroids[i].small;
			auto const hitPos = mAsteroids[i].pos;
			auto const hitVel = mAsteroids[i].vel;
			respawn_asteroid_( i );
			if( !wasSmall )
			{
				spawn_split_asteroid_( hitPos + Vec2f{ 10.f, -8.f }, hitVel + Vec2f{ 70.f, 40.f } );
				spawn_split_asteroid_( hitPos + Vec2f{ -9.f, 6.f }, hitVel + Vec2f{ -60.f, -45.f } );
				hits += 2;
			}
			else
			{
				++hits;
			}
		}
	}

	return hits;
}

bool AsteroidField::collides( Vec2f const& aPos, float aRadius ) const
{
	for( auto const& astr : mAsteroids )
	{
		auto const delta = astr.pos - aPos;
		auto const hitRadius = aRadius + astr.collisionRadius;
		auto const distSq = dot( delta, delta );
		if( distSq <= hitRadius * hitRadius )
			return true;
	}

	return false;
}

void AsteroidField::respawn_asteroid_( std::size_t aIndex )
{
	using Uniform_ = std::uniform_real_distribution<float>;
	using Normal_ = std::normal_distribution<float>;

	Uniform_ xpos{ mBoundsMin.x, mBoundsMax.x };
	Uniform_ ypos{ mBoundsMin.y, mBoundsMax.y };
	Uniform_ angle( 0.f, 2*kPI );
	Normal_ vvel{ 0.f, mInitialSpeed };
	Normal_ rots{ 0.f, mInitialRot };

	auto& astr = mAsteroids[aIndex];

	if( astr.pos.x < mBoundsMin.x )
	{
		astr.pos.x = mBoundsMax.x - mPadding/2.f;
		astr.pos.y = ypos( mRNG );
	}
	else if( astr.pos.x > mBoundsMax.x )
	{
		astr.pos.x = mBoundsMin.x + mPadding/2.f;
		astr.pos.y = ypos( mRNG );
	}
	else if( astr.pos.y < mBoundsMin.y )
	{
		astr.pos.x = xpos( mRNG );
		astr.pos.y = mBoundsMax.y - mPadding/2.f;
	}
	else if( astr.pos.y > mBoundsMax.y )
	{
		astr.pos.x = xpos( mRNG );
		astr.pos.y = mBoundsMin.y + mPadding/2.f;
	}
	else
	{
		astr.pos.x = xpos( mRNG );
		astr.pos.y = ypos( mRNG );
	}

	astr.vel = Vec2f{ vvel( mRNG ), vvel( mRNG ) };
	astr.rot = make_rotation_2d( angle( mRNG ) );
	astr.radpersec = rots( mRNG );
	astr.collisionRadius = 40.f;
	astr.small = false;

	astr.vel.x = std::clamp( astr.vel.x, -mMaximumSpeed, +mMaximumSpeed );
	astr.vel.y = std::clamp( astr.vel.y, -mMaximumSpeed, +mMaximumSpeed );

	mShapes[aIndex] = make_asteroid( mRNG );
}

void AsteroidField::spawn_split_asteroid_( Vec2f const& aPos, Vec2f const& aVelocity )
{
	if( mAsteroids.size() >= mMaxAsteroids )
		return;

	using Uniform_ = std::uniform_real_distribution<float>;
	using Normal_ = std::normal_distribution<float>;

	Uniform_ angle( 0.f, 2*kPI );
	Normal_ rots{ 0.f, mInitialRot * 1.4f };

	Asteroid_ astr;
	astr.pos = aPos;
	astr.vel = aVelocity;
	astr.rot = make_rotation_2d( angle( mRNG ) );
	astr.radpersec = rots( mRNG );
	astr.collisionRadius = 22.f;
	astr.small = true;
	astr.vel.x = std::clamp( astr.vel.x, -mMaximumSpeed * 1.2f, +mMaximumSpeed * 1.2f );
	astr.vel.y = std::clamp( astr.vel.y, -mMaximumSpeed * 1.2f, +mMaximumSpeed * 1.2f );

	mAsteroids.push_back( astr );
	mShapes.emplace_back( make_asteroid( mRNG, 10, 16.f, 2.2f, 0.25f, 1.8f ) );
}

