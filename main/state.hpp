#ifndef STATE_HPP_BE728505_2D00_4E60_9F37_6C58B3569251
#define STATE_HPP_BE728505_2D00_4E60_9F37_6C58B3569251

#include <array>

#include <glad.h> // Make sure <GL/gl.h> isn't being included.
#include <GLFW/glfw3.h>

#include "../vmlib/vec2.hpp"

enum class EInputMode
{
	standard,
	piloting
};

enum class EDifficulty
{
	easy = 0,
	normal = 1,
	hard = 2
};

struct State
{
	EInputMode inputMode = EInputMode::standard;

	// "player" data
	struct Player_
	{
		float angle = 0.f;
		Vec2f velocity = { 0.f, 0.f };
		Vec2f position = { 0.f, 0.f };

		float accelerationMagnitude = 0.f;
	} player;

	// Gameplay
	bool showStartScreen = true;
	bool gameStarted = false;
	bool countdownActive = false;
	float countdownTime = 0.f;
	bool fireRequested = false;
	bool restartRequested = false;
	bool gameOver = false;
	bool thrustKeyHeld = false;
	bool thrustMouseHeld = false;
	EDifficulty difficulty = EDifficulty::normal;
	int lives = 3;
	int score = 0;
	float fireCooldown = 0.f;
	float invulnerabilityTime = 0.f;
	int wave = 1;
	int weaponLevel = 1;
	bool bossSpawned = false;
	float shieldMax = 100.f;
	float shield = 100.f;
	float shieldRegenCooldown = 0.f;
	float shieldRegenRate = 0.f;
	float rapidFireTime = 0.f;
	int comboCount = 0;
	float comboTimer = 0.f;
	float hitFlashTime = 0.f;
	float waveBannerTime = 0.f;
	float screenShakeTime = 0.f;
	float screenShakeStrength = 0.f;
	bool audioEnabled = true;

	// Statistics & progression
	int highScore = 0;
	int totalKills = 0;
	int bossesDefeated = 0;
	float totalPlayTime = 0.f;
	int maxWaveReached = 0;

	// Engine exhaust
	float exhaustTimer = 0.f;

	// Minimap
	bool showMinimap = true;
	bool paused = false;
	bool screenshotRequested = false;
	bool showDebugOverlay = false;

	// FPS tracking
	float fpsAccum = 0.f;
	int fpsFrames = 0;
	float displayFps = 0.f;
	float frameTimeMs = 0.f;
	std::array<float, 120> frameTimeHistory{};
	std::size_t frameTimeHistoryIndex = 0;

	// Score popup
	float slowMotionTime = 0.f;

	// Frame data
	struct Frame_
	{
		float dt = 0.f;
		Vec2f movement = { 0.f, 0.f };
	} thisFrame;

	// Misc.
	GLFWcursor* crosshair = nullptr;
};


void state_update( State&, float aDeltaSeconds );

#endif // STATE_HPP_BE728505_2D00_4E60_9F37_6C58B3569251
