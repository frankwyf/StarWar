#include "state.hpp"

#include <cstdio>
#include <algorithm>

void state_update( State& aState, float aDeltaSeconds )
{
	aState.thisFrame.dt = aDeltaSeconds;
	aState.thisFrame.movement = { 0.f, 0.f };

	if( aState.fireCooldown > 0.f )
		aState.fireCooldown = std::max( 0.f, aState.fireCooldown - aDeltaSeconds );

	if( aState.invulnerabilityTime > 0.f )
		aState.invulnerabilityTime = std::max( 0.f, aState.invulnerabilityTime - aDeltaSeconds );

	if( aState.countdownActive )
		aState.countdownTime = std::max( 0.f, aState.countdownTime - aDeltaSeconds );

	if( aState.shieldRegenCooldown > 0.f )
		aState.shieldRegenCooldown = std::max( 0.f, aState.shieldRegenCooldown - aDeltaSeconds );

	if( aState.rapidFireTime > 0.f )
		aState.rapidFireTime = std::max( 0.f, aState.rapidFireTime - aDeltaSeconds );

	if( aState.comboTimer > 0.f )
		aState.comboTimer = std::max( 0.f, aState.comboTimer - aDeltaSeconds );
	else
		aState.comboCount = 0;

	if( aState.hitFlashTime > 0.f )
		aState.hitFlashTime = std::max( 0.f, aState.hitFlashTime - aDeltaSeconds );

	if( aState.waveBannerTime > 0.f )
		aState.waveBannerTime = std::max( 0.f, aState.waveBannerTime - aDeltaSeconds );

	if( aState.screenShakeTime > 0.f )
	{
		aState.screenShakeTime = std::max( 0.f, aState.screenShakeTime - aDeltaSeconds );
		aState.screenShakeStrength = std::max( 0.f, aState.screenShakeStrength - aDeltaSeconds * 18.f );
	}
	else
	{
		aState.screenShakeStrength = 0.f;
	}

	if( aState.slowMotionTime > 0.f )
		aState.slowMotionTime = std::max( 0.f, aState.slowMotionTime - aDeltaSeconds );

	// FPS tracking
	aState.fpsAccum += aDeltaSeconds;
	++aState.fpsFrames;
	if( aState.fpsAccum >= 0.5f )
	{
		aState.displayFps = float(aState.fpsFrames) / aState.fpsAccum;
		aState.fpsAccum = 0.f;
		aState.fpsFrames = 0;
	}

	// Play time
	if( !aState.showStartScreen && !aState.gameOver && !aState.countdownActive )
		aState.totalPlayTime += aDeltaSeconds;

	// Exhaust timer
	aState.exhaustTimer += aDeltaSeconds;

	aState.player.accelerationMagnitude = (aState.thrustKeyHeld || aState.thrustMouseHeld) ? 500.f : 0.f;

	if( aState.showStartScreen || aState.gameOver )
	{
		aState.player.velocity = { 0.f, 0.f };
		return;
	}

	Vec2f const acceleration{
		aState.player.accelerationMagnitude * std::cos( aState.player.angle ),
		aState.player.accelerationMagnitude * std::sin( aState.player.angle )
	};

	aState.player.velocity += acceleration * aDeltaSeconds;
	aState.player.velocity *= 0.99f;

	Vec2f const movement = aState.player.velocity * aDeltaSeconds;
	aState.player.position += movement;
	aState.thisFrame.movement = movement;
}
