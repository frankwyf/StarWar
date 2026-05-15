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
