#include <glad.h>
#include <GLFW/glfw3.h>

#include <random>
#include <typeinfo>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <string_view>
#include <cctype>

#include <cstdio>
#include <cstdlib>

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"
#include "../draw2d/shape.hpp"

#include "../support/error.hpp"
#include "../support/context.hpp"
#include "../support/runconfig.hpp"

#include "../vmlib/vec2.hpp"
#include "../vmlib/mat22.hpp"

#include "defaults.hpp"
#include "state.hpp"
#include "spaceship.hpp"
#include "background.hpp"
#include "asteroid_field.hpp"

namespace
{
	constexpr char const* kWindowTitle = "StarWar";
	constexpr float kBulletSpeed = 950.f;
	constexpr float kBulletLife = 1.2f;
	constexpr float kBulletRadius = 20.f;
	constexpr float kPlayerCollisionRadius = 52.f;
	constexpr float kRespawnInvulnerability = 1.5f;
	constexpr float kEnemyBulletSpeed = 300.f;
	constexpr float kPlayerBulletHitRadius = 12.f;
	constexpr float kStartCountdownSeconds = 3.f;

	float difficulty_enemy_speed_scale_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return 0.82f;
		case EDifficulty::hard: return 1.22f;
		case EDifficulty::normal:
		default: return 1.f;
		}
	}

	float difficulty_enemy_fire_scale_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return 1.35f;
		case EDifficulty::hard: return 0.8f;
		case EDifficulty::normal:
		default: return 1.f;
		}
	}

	int difficulty_extra_start_lives_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return 1;
		case EDifficulty::hard: return -1;
		case EDifficulty::normal:
		default: return 0;
		}
	}

	float difficulty_start_shield_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return 130.f;
		case EDifficulty::hard: return 85.f;
		case EDifficulty::normal:
		default: return 100.f;
		}
	}

	float difficulty_shield_regen_rate_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return 12.f;
		case EDifficulty::hard: return 6.f;
		case EDifficulty::normal:
		default: return 8.5f;
		}
	}

	int difficulty_wave_enemy_bonus_( EDifficulty d )
	{
		switch( d )
		{
		case EDifficulty::easy: return -1;
		case EDifficulty::hard: return 1;
		case EDifficulty::normal:
		default: return 0;
		}
	}

	void glfw_callback_error_( int, char const* );

	void glfw_callback_key_( GLFWwindow*, int, int, int, int );
	void glfw_callback_button_( GLFWwindow*, int, int, int );
	void glfw_callback_motion_( GLFWwindow*, double, double );

	struct Bullet
	{
		Vec2f pos;
		Vec2f vel;
		float life = kBulletLife;
		bool enemy = false;
	};

	struct Enemy
	{
		Vec2f pos;
		Vec2f vel;
		float angle = 0.f;
		float fireCooldown = 0.f;
		float hp = 1.f;
		bool boss = false;
	};

	struct GLFWCleanupHelper
	{
		~GLFWCleanupHelper();
	};
	struct GLFWWindowDeleter
	{
		~GLFWWindowDeleter();
		GLFWwindow* window;
	};

	const char* glyph5x7_( char c )
	{
		switch( std::toupper(static_cast<unsigned char>(c)) )
		{
		case 'A': return "01110" "10001" "10001" "11111" "10001" "10001" "10001";
		case 'B': return "11110" "10001" "10001" "11110" "10001" "10001" "11110";
		case 'C': return "01110" "10001" "10000" "10000" "10000" "10001" "01110";
		case 'D': return "11110" "10001" "10001" "10001" "10001" "10001" "11110";
		case 'E': return "11111" "10000" "10000" "11110" "10000" "10000" "11111";
		case 'F': return "11111" "10000" "10000" "11110" "10000" "10000" "10000";
		case 'G': return "01110" "10001" "10000" "10111" "10001" "10001" "01110";
		case 'H': return "10001" "10001" "10001" "11111" "10001" "10001" "10001";
		case 'I': return "11111" "00100" "00100" "00100" "00100" "00100" "11111";
		case 'J': return "00111" "00010" "00010" "00010" "00010" "10010" "01100";
		case 'K': return "10001" "10010" "10100" "11000" "10100" "10010" "10001";
		case 'L': return "10000" "10000" "10000" "10000" "10000" "10000" "11111";
		case 'M': return "10001" "11011" "10101" "10101" "10001" "10001" "10001";
		case 'N': return "10001" "11001" "10101" "10011" "10001" "10001" "10001";
		case 'O': return "01110" "10001" "10001" "10001" "10001" "10001" "01110";
		case 'P': return "11110" "10001" "10001" "11110" "10000" "10000" "10000";
		case 'Q': return "01110" "10001" "10001" "10001" "10101" "10010" "01101";
		case 'R': return "11110" "10001" "10001" "11110" "10100" "10010" "10001";
		case 'S': return "01111" "10000" "10000" "01110" "00001" "00001" "11110";
		case 'T': return "11111" "00100" "00100" "00100" "00100" "00100" "00100";
		case 'U': return "10001" "10001" "10001" "10001" "10001" "10001" "01110";
		case 'V': return "10001" "10001" "10001" "10001" "01010" "01010" "00100";
		case 'W': return "10001" "10001" "10001" "10101" "10101" "11011" "10001";
		case 'X': return "10001" "01010" "00100" "00100" "00100" "01010" "10001";
		case 'Y': return "10001" "01010" "00100" "00100" "00100" "00100" "00100";
		case 'Z': return "11111" "00001" "00010" "00100" "01000" "10000" "11111";
		case '0': return "01110" "10001" "10011" "10101" "11001" "10001" "01110";
		case '1': return "00100" "01100" "00100" "00100" "00100" "00100" "01110";
		case '2': return "01110" "10001" "00001" "00010" "00100" "01000" "11111";
		case '3': return "11110" "00001" "00001" "01110" "00001" "00001" "11110";
		case '4': return "00010" "00110" "01010" "10010" "11111" "00010" "00010";
		case '5': return "11111" "10000" "10000" "11110" "00001" "00001" "11110";
		case '6': return "01110" "10000" "10000" "11110" "10001" "10001" "01110";
		case '7': return "11111" "00001" "00010" "00100" "01000" "01000" "01000";
		case '8': return "01110" "10001" "10001" "01110" "10001" "10001" "01110";
		case '9': return "01110" "10001" "10001" "01111" "00001" "00001" "01110";
		case ':': return "00000" "00100" "00100" "00000" "00100" "00100" "00000";
		case '/': return "00001" "00010" "00100" "01000" "10000" "00000" "00000";
		case '-': return "00000" "00000" "00000" "11111" "00000" "00000" "00000";
		case '.': return "00000" "00000" "00000" "00000" "00000" "00110" "00110";
		case '(': return "00010" "00100" "01000" "01000" "01000" "00100" "00010";
		case ')': return "01000" "00100" "00010" "00010" "00010" "00100" "01000";
		case ' ': return "00000" "00000" "00000" "00000" "00000" "00000" "00000";
		default:  return "00000" "00000" "00000" "00000" "00000" "00000" "00000";
		}
	}

	void draw_text_5x7_( Surface& s, Vec2f pos, int scale, ColorU8_sRGB col, std::string_view txt )
	{
		float x = pos.x;
		for( char ch : txt )
		{
			auto glyph = glyph5x7_( ch );
			for( int r = 0; r < 7; ++r )
			{
				for( int c = 0; c < 5; ++c )
				{
					if( glyph[r*5 + c] == '1' )
					{
						int rr = 6 - r;
						Vec2f p0{ x + float(c*scale), pos.y + float(rr*scale) };
						Vec2f p1{ p0.x + float(scale), p0.y + float(scale) };
						draw_rectangle_solid( s, p0, p1, col );
					}
				}
			}
			x += float(6 * scale);
		}
	}
}

int main( int aArgc, char* aArgv[] ) try
{
    MessageBoxA(nullptr, "StarWar 启动成功，已进入 main()。", "StarWar 启动诊断", MB_OK | MB_ICONINFORMATION);

	// Parse command line arguments
	RuntimeConfig const config = parse_command_line( aArgc, aArgv );

	// Initialize GLFW
	if( GLFW_TRUE != glfwInit() )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwInit() failed with '%s' (%d)", msg, ecode );
	}

	// Ensure that we call glfwTerminate() at the end of the program.
	GLFWCleanupHelper cleanupHelper;

	// Configure GLFW and create window
	glfwSetErrorCallback( &glfw_callback_error_ );

	glfwWindowHint( GLFW_SRGB_CAPABLE, GLFW_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );

	//glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE ); // Allow resizing! Do not change this!

#	if !defined(__APPLE__)
	// Most platforms will support OpenGL 4.3
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
#	else // defined(__APPLE__)
	// Apple has at most OpenGL 4.1, so don't ask for something newer.
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
#	endif // ~ __APPLE__
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );

#	if !defined(NDEBUG)
	// When building in debug mode, request an OpenGL debug context. This
	// enables additional debugging features. However, this can carry extra
	// overheads. We therefore do not do this for release builds.
	glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE );
#	endif // ~ !NDEBUG

	GLFWwindow* window = glfwCreateWindow(
		int(config.initialWindowWidth),
		int(config.initialWindowHeight),
		kWindowTitle,
		nullptr, nullptr
	);

	if( !window )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwCreateWindow() failed with '%s' (%d)", msg, ecode );
	}

	GLFWWindowDeleter windowDeleter{ window };

	// Runtime state
	State state;
	std::vector<Bullet> bullets;
	bullets.reserve( 256 );
	std::vector<Enemy> enemies;
	enemies.reserve( 16 );
	std::uniform_real_distribution<float> unit01( 0.f, 1.f );

	// Cursors
	state.crosshair = glfwCreateStandardCursor( GLFW_CROSSHAIR_CURSOR );

	// Set up event handling
	glfwSetWindowUserPointer( window, &state );

	glfwSetKeyCallback( window, &glfw_callback_key_ );
	glfwSetMouseButtonCallback( window, &glfw_callback_button_ );
	glfwSetCursorPosCallback( window, &glfw_callback_motion_ );

	// Set up drawing stuff
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 ); // V-Sync is on.

	// Get actual framebuffer size.
	// This can be different from the window size, as standard window
	// decorations (title bar, borders, ...) may be included in the window size
	// but not be part of the drawable surface area.
	int iwidth, iheight;
	glfwGetFramebufferSize( window, &iwidth, &iheight );

	float wscale = 1.f, hscale = 1.f;
#	if defined(__APPLE__)
	// HACK: Window content scaling on MacOS.
	//
	// This is a workaround for MacOS, where scaling affects retina displays.
	// Windows technically also does content scaling, but it seems to do this
	// more transparently. Either way, the behaviour doesn't seem to be
	// consistent across the platforms, though, which is slightly unfortunate.
	// (And not having a (retina) Mac to test on makes figuring this out a tad
	// more tricky.)
	glfwGetWindowContentScale( window, &wscale, &hscale );
#	endif

	assert( iwidth >= 0 && iheight >= 0 );
	auto fbwidth = std::uint32_t(iwidth / wscale) >> config.framebufferScaleShift;
	auto fbheight = std::uint32_t(iheight / hscale) >> config.framebufferScaleShift;

	Context context( fbwidth, fbheight );
	Surface surface( fbwidth, fbheight );

	glViewport( 0, 0, iwidth, iheight );

	// Resources
	RNG rng( std::random_device{}() );

	Background background( rng, fbwidth, fbheight );
	AsteroidField asteroids( rng, fbwidth, fbheight );

	auto const spaceship = make_spaceship_shape();


	// Main loop
	auto lastUpdateTime = Clock::now();

	while( !glfwWindowShouldClose( window ) )
	{
		// Let GLFW process events
		glfwPollEvents();
		
		// Check if window was resized.
		{
			int nwidth, nheight;
			glfwGetFramebufferSize( window, &nwidth, &nheight );

			if( 0 == nwidth || 0 == nheight )
			{
				// Window minimized? Pause until it is unminimized.
				// This is a bit of a hack.
				do
				{
					glfwWaitEvents();
					glfwGetFramebufferSize( window, &nwidth, &nheight );
				} while( 0 == nwidth || 0 == nheight );
			}

			if( iwidth != nwidth || iheight != nheight )
			{
				iwidth = nwidth;
				iheight = nheight;

				float ws = 1.f, hs = 1.f;
#				if defined(__APPLE__)
				glfwGetWindowContentScale( window, &ws, &hs );
#				endif

				glViewport( 0, 0, iwidth, iheight );

				assert( iwidth >= 0 && iheight >= 0 );
				fbwidth = std::uint32_t(iwidth / ws) >> config.framebufferScaleShift;
				fbheight = std::uint32_t(iheight / hs) >> config.framebufferScaleShift;

				// Resize things
				context.resize( fbwidth, fbheight );

				surface = Surface( fbwidth, fbheight );
				background.resize( fbwidth, fbheight );
				asteroids.resize( fbwidth, fbheight );
			}
		}

		// Update state
		auto const now = Clock::now();
		auto const dt = std::chrono::duration_cast<Secondsf>(now - lastUpdateTime).count();
		lastUpdateTime = now;

		if( state.restartRequested )
		{
			auto* cursor = state.crosshair;
			auto const selectedDifficulty = state.difficulty;
			state = State{};
			state.crosshair = cursor;
			state.difficulty = selectedDifficulty;
			state.inputMode = EInputMode::piloting;
			state.showStartScreen = true;
			glfwSetCursor( window, nullptr );
			bullets.clear();
			enemies.clear();
		}

		state_update( state, dt );

		if( state.countdownActive && state.countdownTime <= 0.f )
		{
			state.countdownActive = false;
		}

		if( !state.showStartScreen && !state.gameOver && !state.countdownActive && state.fireRequested && state.fireCooldown <= 0.f )
		{
			Vec2f dir{ std::cos( state.player.angle ), std::sin( state.player.angle ) };
			Vec2f muzzle = Vec2f{ fbwidth*0.5f, fbheight*0.5f } + dir * 28.f;
			bullets.push_back( Bullet{ muzzle, dir * kBulletSpeed, kBulletLife, false } );
			if( state.weaponLevel >= 2 )
			{
				Mat22f lrot = make_rotation_2d( 0.16f );
				Mat22f rrot = make_rotation_2d( -0.16f );
				bullets.push_back( Bullet{ muzzle, (lrot * dir) * (kBulletSpeed * 0.9f), kBulletLife, false } );
				bullets.push_back( Bullet{ muzzle, (rrot * dir) * (kBulletSpeed * 0.9f), kBulletLife, false } );
			}
			state.fireCooldown = state.weaponLevel >= 3 ? 0.09f : 0.18f;
		}
		state.fireRequested = false;

		if( !state.showStartScreen && !state.gameOver && !state.countdownActive && enemies.empty() )
		{
			auto const enemySpeedScale = difficulty_enemy_speed_scale_( state.difficulty );
			auto const enemyFireScale = difficulty_enemy_fire_scale_( state.difficulty );
			if( state.wave % 4 == 0 )
			{
				Enemy boss;
				boss.boss = true;
				boss.hp = 14.f + state.wave;
				boss.pos = { fbwidth + 160.f, fbheight * 0.5f };
				boss.vel = { -90.f * enemySpeedScale, 0.f };
				boss.fireCooldown = 1.0f * enemyFireScale;
				enemies.push_back( boss );
				state.bossSpawned = true;
			}
			else
			{
				int count = std::max( 1, 2 + state.wave + difficulty_wave_enemy_bonus_( state.difficulty ) );
				std::uniform_real_distribution<float> sideY( 100.f, std::max( 120.f, float(fbheight) - 100.f ) );
				for( int i = 0; i < count; ++i )
				{
					Enemy e;
					e.pos = { (i % 2 == 0 ? -40.f : fbwidth + 40.f), sideY(rng) };
					e.vel = { ((i % 2 == 0 ? 100.f : -100.f) * (1.f + 0.06f * state.wave)) * enemySpeedScale, ((unit01(rng) - 0.5f) * 30.f) * enemySpeedScale };
					e.fireCooldown = (0.5f + unit01(rng)) * enemyFireScale;
					e.hp = 1.f;
					enemies.push_back( e );
				}
			}
		}

		if( !state.showStartScreen && !state.countdownActive )
		{
			auto const enemySpeedScale = difficulty_enemy_speed_scale_( state.difficulty );
			auto const enemyFireScale = difficulty_enemy_fire_scale_( state.difficulty );
			for( auto& e : enemies )
			{
				if( state.gameOver ) break;
				Vec2f playerScreen{ fbwidth * 0.5f, fbheight * 0.5f };
				Vec2f toPlayer = playerScreen - e.pos;
				float len = length( toPlayer );
				if( len > 1.f )
					toPlayer /= len;
				e.angle = std::atan2( toPlayer.y, toPlayer.x );

				e.pos += e.vel * state.thisFrame.dt - state.thisFrame.movement;
				if( e.boss )
				{
					e.pos.y += std::sin( now.time_since_epoch().count() * 1e-9f * 2.f ) * 20.f * state.thisFrame.dt;
					e.fireCooldown -= state.thisFrame.dt;
					if( e.fireCooldown <= 0.f )
					{
						Mat22f l = make_rotation_2d( 0.24f );
						Mat22f r = make_rotation_2d( -0.24f );
						bullets.push_back( Bullet{ e.pos, toPlayer * (kEnemyBulletSpeed * enemySpeedScale), 2.2f, true } );
						bullets.push_back( Bullet{ e.pos, (l * toPlayer) * (kEnemyBulletSpeed * 0.92f * enemySpeedScale), 2.2f, true } );
						bullets.push_back( Bullet{ e.pos, (r * toPlayer) * (kEnemyBulletSpeed * 0.92f * enemySpeedScale), 2.2f, true } );
						e.fireCooldown = 0.72f * enemyFireScale;
					}
				}
				else
				{
					e.fireCooldown -= state.thisFrame.dt;
					if( e.fireCooldown <= 0.f )
					{
						float jitter = (unit01(rng) - 0.5f) * 0.30f;
						Mat22f miss = make_rotation_2d( jitter );
						bullets.push_back( Bullet{ e.pos, (miss * toPlayer) * (kEnemyBulletSpeed * 0.95f * enemySpeedScale), 1.9f, true } );
						e.fireCooldown = (1.35f + unit01(rng) * 0.7f) * enemyFireScale;
					}
				}
			}
		}

		for( auto it = bullets.begin(); it != bullets.end(); )
		{
			it->life -= state.thisFrame.dt;
			it->pos += it->vel * state.thisFrame.dt - state.thisFrame.movement;

			bool consumed = false;
			if( !state.showStartScreen && !it->enemy )
			{
				auto const hits = asteroids.hit_test_and_destroy( it->pos, kBulletRadius );
				if( hits > 0 )
				{
					state.score += int(hits) * 10;
					consumed = true;
				}
				for( auto eit = enemies.begin(); eit != enemies.end() && !consumed; )
				{
					float hitR = eit->boss ? 46.f : 20.f;
					auto d = eit->pos - it->pos;
					if( dot(d,d) <= hitR*hitR )
					{
						eit->hp -= state.weaponLevel >= 3 ? 1.4f : 1.f;
						consumed = true;
						if( eit->hp <= 0.f )
						{
							state.score += eit->boss ? 300 : 40;
							eit = enemies.erase( eit );
						}
						else
						{
							++eit;
						}
					}
					else
					{
						++eit;
					}
				}
			}
			else if( !state.showStartScreen && !state.gameOver && !state.countdownActive && state.invulnerabilityTime <= 0.f && it->enemy )
				{
					auto d = it->pos - Vec2f{ fbwidth*0.5f, fbheight*0.5f };
					if( dot(d,d) <= kPlayerBulletHitRadius * kPlayerBulletHitRadius )
					{
						if( state.shield > 0.f )
						{
							state.shield = std::max( 0.f, state.shield - 22.f );
							state.shieldRegenCooldown = 1.8f;
							state.invulnerabilityTime = 0.2f;
						}
						else
						{
							--state.lives;
							state.invulnerabilityTime = kRespawnInvulnerability;
							state.player.velocity = { 0.f, 0.f };
							if( state.lives <= 0 )
								state.gameOver = true;
						}
						consumed = true;
					}
				}

			if( it->life <= 0.f || consumed )
				it = bullets.erase( it );
			else
				++it;
		}

		if( !state.showStartScreen && !state.gameOver && !state.countdownActive && state.shieldRegenCooldown <= 0.f && state.shield < state.shieldMax )
		{
			state.shield = std::min( state.shieldMax, state.shield + state.shieldRegenRate * state.thisFrame.dt );
		}

		background.update( state.player.position, state.thisFrame.movement );
		asteroids.update( state.thisFrame.dt, state.thisFrame.movement );

		if( !state.showStartScreen && !state.gameOver && !state.countdownActive && state.invulnerabilityTime <= 0.f )
		{
			auto absorb_or_hit_life = [&]( float shieldDamage )
			{
				if( state.shield > 0.f )
				{
					state.shield = std::max( 0.f, state.shield - shieldDamage );
					state.shieldRegenCooldown = 2.0f;
					state.invulnerabilityTime = 0.24f;
				}
				else
				{
					--state.lives;
					state.invulnerabilityTime = kRespawnInvulnerability;
					state.player.velocity = { 0.f, 0.f };
					if( state.lives <= 0 )
						state.gameOver = true;
				}
			};

			if( asteroids.collides( Vec2f{ fbwidth*0.5f, fbheight*0.5f }, kPlayerCollisionRadius ) )
			{
				absorb_or_hit_life( 34.f );
			}
			for( auto const& e : enemies )
			{
				auto d = e.pos - Vec2f{ fbwidth*0.5f, fbheight*0.5f };
				float rr = e.boss ? 62.f : 28.f;
				if( dot(d,d) <= rr*rr )
				{
					absorb_or_hit_life( e.boss ? 48.f : 26.f );
					break;
				}
			}
		}

		if( !state.showStartScreen && !state.gameOver && !state.countdownActive && enemies.empty() )
		{
			++state.wave;
			if( state.wave >= 2 ) state.weaponLevel = 2;
			if( state.wave >= 4 ) state.weaponLevel = 3;
			state.shield = std::min( state.shieldMax, state.shield + 22.f );
		}

		// Draw scene
		surface.clear();
		background.draw( surface );
		asteroids.draw( surface );

		for( auto const& e : enemies )
		{
			auto const er = make_rotation_2d( e.angle );
			auto const ec = e.boss ? ColorF{ 0.8f, 0.2f, 0.2f } : ColorF{ 0.8f, 0.5f, 0.2f };
			spaceship.draw( surface, ec, er, e.pos );
			if( e.boss )
			{
				draw_rectangle_outline( surface, e.pos + Vec2f{-40.f,-46.f}, e.pos + Vec2f{40.f,-40.f}, ColorU8_sRGB{200,200,200} );
				draw_rectangle_solid( surface, e.pos + Vec2f{-38.f,-44.f}, e.pos + Vec2f{-38.f + 76.f * (e.hp/(14.f + state.wave)), -42.f}, ColorU8_sRGB{220,80,80} );
			}
		}

		for( auto const& bullet : bullets )
		{
			draw_rectangle_solid(
				surface,
				bullet.pos - Vec2f{ 2.f, 2.f },
				bullet.pos + Vec2f{ 2.f, 2.f },
				bullet.enemy ? ColorU8_sRGB{ 255, 90, 90 } : ColorU8_sRGB{ 255, 230, 120 }
			);
		}

		auto const rot = make_rotation_2d( state.player.angle );
		auto const offs = Vec2f{ fbwidth*0.5f, fbheight*0.5f };
		auto const shipColor = state.invulnerabilityTime > 0.f
			? ColorF{ 0.9f, 0.8f, 0.2f }
			: ColorF{ 0.2f, 0.4f, 0.7f };
		spaceship.draw( surface, shipColor, rot, offs );

		float const scoreBar = std::min( 1.f, state.score / 1200.f );
		draw_rectangle_outline( surface, { 20.f, 20.f }, { 220.f, 36.f }, ColorU8_sRGB{ 180, 180, 180 } );
		draw_rectangle_solid( surface, { 22.f, 22.f }, { 22.f + 196.f * scoreBar, 34.f }, ColorU8_sRGB{ 90, 200, 120 } );

		float const shieldRatio = state.shieldMax > 0.f ? std::clamp( state.shield / state.shieldMax, 0.f, 1.f ) : 0.f;
		draw_rectangle_outline( surface, { 20.f, 42.f }, { 220.f, 58.f }, ColorU8_sRGB{ 120, 170, 255 } );
		draw_rectangle_solid( surface, { 22.f, 44.f }, { 22.f + 196.f * shieldRatio, 56.f }, ColorU8_sRGB{ 70, 140, 240 } );
		draw_text_5x7_( surface, { 226.f, 45.f }, 1, ColorU8_sRGB{ 160, 200, 255 }, "SHIELD" );

		for( int i = 0; i < state.lives; ++i )
		{
			float x0 = float(fbwidth) - 22.f - i * 22.f;
			draw_rectangle_solid( surface, { x0 - 14.f, 20.f }, { x0, 34.f }, ColorU8_sRGB{ 220, 80, 80 } );
		}

		if( state.showStartScreen )
		{
			Vec2f const pmin{ 60.f, 60.f };
			Vec2f const pmax{ float(fbwidth) - 60.f, float(fbheight) - 60.f };
			draw_rectangle_solid( surface, pmin, pmax, ColorU8_sRGB{ 14, 18, 34 } );
			draw_rectangle_outline( surface, pmin, pmax, ColorU8_sRGB{ 120, 150, 235 } );

			draw_rectangle_solid( surface, { pmin.x + 16.f, pmin.y + 16.f }, { pmax.x - 16.f, pmin.y + 64.f }, ColorU8_sRGB{ 24, 34, 70 } );
			draw_rectangle_outline( surface, { pmin.x + 16.f, pmin.y + 16.f }, { pmax.x - 16.f, pmin.y + 64.f }, ColorU8_sRGB{ 145, 180, 255 } );
			draw_text_5x7_( surface, { pmin.x + 28.f, pmin.y + 30.f }, 3, ColorU8_sRGB{ 236, 243, 255 }, "STARWAR - HOW TO PLAY" );

			float const leftX = pmin.x + 26.f;
			float const rightX = pmin.x + (pmax.x - pmin.x) * 0.54f;
			float const topY = pmin.y + 86.f;

			draw_rectangle_outline( surface, { leftX - 10.f, topY - 8.f }, { rightX - 18.f, pmax.y - 80.f }, ColorU8_sRGB{ 88, 118, 180 } );
			draw_rectangle_outline( surface, { rightX - 10.f, topY - 8.f }, { pmax.x - 26.f, pmax.y - 80.f }, ColorU8_sRGB{ 88, 118, 180 } );

			draw_text_5x7_( surface, { leftX, topY }, 2, ColorU8_sRGB{ 170, 210, 255 }, "CORE CONTROLS" );
			draw_text_5x7_( surface, { leftX, topY + 28.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "ENTER: START BATTLE" );
			draw_text_5x7_( surface, { leftX, topY + 52.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "MOUSE: AIM" );
			draw_text_5x7_( surface, { leftX, topY + 76.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "LEFT MOUSE: FIRE" );
			draw_text_5x7_( surface, { leftX, topY + 100.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "W OR UP: THRUST" );
			draw_text_5x7_( surface, { leftX, topY + 124.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "RIGHT MOUSE: THRUST" );
			draw_text_5x7_( surface, { leftX, topY + 148.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "SPACE: TOGGLE PILOT MODE" );
			draw_text_5x7_( surface, { leftX, topY + 172.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "R: RESTART   ESC: QUIT" );

			draw_text_5x7_( surface, { rightX, topY }, 2, ColorU8_sRGB{ 170, 210, 255 }, "GAMEPLAY LOOP" );
			draw_text_5x7_( surface, { rightX, topY + 28.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "1. SURVIVE ASTEROIDS AND ENEMY FIRE" );
			draw_text_5x7_( surface, { rightX, topY + 52.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "2. CLEAR ENEMIES TO ADVANCE WAVES" );
			draw_text_5x7_( surface, { rightX, topY + 76.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "3. EVERY 4TH WAVE SPAWNS A BOSS" );
			draw_text_5x7_( surface, { rightX, topY + 100.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "4. SCORE UPGRADES YOUR FIREPOWER" );
			draw_text_5x7_( surface, { rightX, topY + 124.f }, 2, ColorU8_sRGB{ 250, 210, 90 }, "DIFFICULTY: 1 EASY  2 NORMAL  3 HARD" );
			draw_text_5x7_( surface, { rightX, topY + 148.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "COUNTDOWN START + SHIELD BAR ENABLED" );
			draw_text_5x7_( surface, { rightX, topY + 172.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "CLEARING WAVES RESTORES SHIELD" );

			draw_rectangle_solid( surface, { pmin.x + 22.f, pmax.y - 56.f }, { pmax.x - 22.f, pmax.y - 24.f }, ColorU8_sRGB{ 36, 60, 120 } );
			draw_rectangle_outline( surface, { pmin.x + 22.f, pmax.y - 56.f }, { pmax.x - 22.f, pmax.y - 24.f }, ColorU8_sRGB{ 170, 210, 255 } );
			char difficultyHint[64] = {};
			std::snprintf( difficultyHint, sizeof(difficultyHint), "PRESS ENTER TO START (%s)", state.difficulty == EDifficulty::easy ? "EASY" : (state.difficulty == EDifficulty::hard ? "HARD" : "NORMAL") );
			draw_text_5x7_( surface, { pmin.x + 120.f, pmax.y - 48.f }, 2, ColorU8_sRGB{ 250, 235, 140 }, difficultyHint );
		}

		if( state.countdownActive && !state.showStartScreen && !state.gameOver )
		{
			draw_rectangle_solid(
				surface,
				{ fbwidth * 0.5f - 180.f, fbheight * 0.5f - 58.f },
				{ fbwidth * 0.5f + 180.f, fbheight * 0.5f + 58.f },
				ColorU8_sRGB{ 18, 26, 58 }
			);
			draw_rectangle_outline(
				surface,
				{ fbwidth * 0.5f - 180.f, fbheight * 0.5f - 58.f },
				{ fbwidth * 0.5f + 180.f, fbheight * 0.5f + 58.f },
				ColorU8_sRGB{ 170, 210, 255 }
			);
			int countValue = std::max( 1, int(std::ceil( state.countdownTime )) );
			char countdownText[64] = {};
			std::snprintf( countdownText, sizeof(countdownText), "MISSION START IN %d", countValue );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 146.f, fbheight * 0.5f - 10.f }, 3, ColorU8_sRGB{ 245, 232, 140 }, countdownText );
		}

		if( state.gameOver )
		{
			draw_rectangle_solid(
				surface,
				{ fbwidth * 0.5f - 170.f, fbheight * 0.5f - 35.f },
				{ fbwidth * 0.5f + 170.f, fbheight * 0.5f + 35.f },
				ColorU8_sRGB{ 120, 20, 20 }
			);
			draw_rectangle_outline(
				surface,
				{ fbwidth * 0.5f - 170.f, fbheight * 0.5f - 35.f },
				{ fbwidth * 0.5f + 170.f, fbheight * 0.5f + 35.f },
				ColorU8_sRGB{ 255, 120, 120 }
			);
			draw_text_5x7_( surface, { fbwidth * 0.5f - 110.f, fbheight * 0.5f - 24.f }, 4, ColorU8_sRGB{ 255, 230, 230 }, "GAME OVER" );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 116.f, fbheight * 0.5f + 10.f }, 2, ColorU8_sRGB{ 255, 180, 180 }, "PRESS R TO RESTART" );
		}

		context.draw( surface );

		char title[320] = {};
		if( state.showStartScreen )
			std::snprintf( title, sizeof(title), "%s | Difficulty: %s (1/2/3) | Start: ENTER | Toggle pilot: SPACE | Aim: Mouse | Thrust: W/UP or Right Mouse | Fire: Left Mouse", kWindowTitle, state.difficulty == EDifficulty::easy ? "Easy" : (state.difficulty == EDifficulty::hard ? "Hard" : "Normal") );
		else if( state.countdownActive )
			std::snprintf( title, sizeof(title), "%s | Difficulty: %s | Launch in %.1fs", kWindowTitle, state.difficulty == EDifficulty::easy ? "Easy" : (state.difficulty == EDifficulty::hard ? "Hard" : "Normal"), state.countdownTime );
		else
			std::snprintf( title, sizeof(title), "%s | Score: %d | Lives: %d | Wave: %d | Weapon: Lv%d | Difficulty: %s%s", kWindowTitle, state.score, state.lives, state.wave, state.weaponLevel, state.difficulty == EDifficulty::easy ? "Easy" : (state.difficulty == EDifficulty::hard ? "Hard" : "Normal"), state.gameOver ? " | GAME OVER (R to restart)" : "" );
		glfwSetWindowTitle( window, title );

		glfwSwapBuffers( window );

	}

	// Cleanup.
	// For now, all objects are automatically cleaned up when they go out of
	// scope.
	
	return 0;
}
catch( std::exception const& e )
{
    MessageBoxA(nullptr, e.what(), "StarWar 崩溃异常", MB_OK | MB_ICONERROR);
    return 1;
}
catch(...)
{
    MessageBoxA(nullptr, "Unknown exception!", "StarWar 崩溃异常", MB_OK | MB_ICONERROR);
    return 2;
}


namespace
{
	void glfw_callback_error_( int aErrNum, char const* aErrDesc )
	{
		std::fprintf( stderr, "GLFW error: %s (%d)\n", aErrDesc, aErrNum );
	}

	void glfw_callback_key_( GLFWwindow* aWindow, int aKey, int, int aAction, int )
	{
		if( GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction )
		{
			glfwSetWindowShouldClose( aWindow, GLFW_TRUE );
			return;
		}

		auto* state = static_cast<State*>(glfwGetWindowUserPointer( aWindow ));
		assert( state );

		if( GLFW_KEY_ENTER == aKey && GLFW_PRESS == aAction )
		{
			if( state->showStartScreen )
			{
				state->showStartScreen = false;
				state->gameStarted = true;
				state->countdownActive = true;
				state->countdownTime = kStartCountdownSeconds;
				state->lives = std::max( 1, 3 + difficulty_extra_start_lives_( state->difficulty ) );
				state->shieldMax = difficulty_start_shield_( state->difficulty );
				state->shield = state->shieldMax;
				state->shieldRegenRate = difficulty_shield_regen_rate_( state->difficulty );
				state->shieldRegenCooldown = 1.2f;
			}
			state->inputMode = EInputMode::piloting;
			glfwSetCursor( aWindow, state->crosshair );
			return;
		}

		if( GLFW_KEY_R == aKey && GLFW_PRESS == aAction )
		{
			state->restartRequested = true;
			return;
		}

		if( GLFW_KEY_W == aKey || GLFW_KEY_UP == aKey )
		{
			if( GLFW_PRESS == aAction )
				state->thrustKeyHeld = true;
			else if( GLFW_RELEASE == aAction )
				state->thrustKeyHeld = false;
		}

		if( state->showStartScreen )
		{
			if( GLFW_PRESS == aAction )
			{
				if( GLFW_KEY_1 == aKey || GLFW_KEY_KP_1 == aKey )
					state->difficulty = EDifficulty::easy;
				else if( GLFW_KEY_2 == aKey || GLFW_KEY_KP_2 == aKey )
					state->difficulty = EDifficulty::normal;
				else if( GLFW_KEY_3 == aKey || GLFW_KEY_KP_3 == aKey )
					state->difficulty = EDifficulty::hard;
			}
		}

		if( EInputMode::standard == state->inputMode )
		{
			if( GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction )
			{
				state->inputMode = EInputMode::piloting;
				glfwSetCursor( aWindow, state->crosshair );
			}
		}
		else if( EInputMode::piloting == state->inputMode )
		{
			if( GLFW_KEY_SPACE == aKey && GLFW_PRESS == aAction )
			{
				state->inputMode = EInputMode::standard;
				glfwSetCursor( aWindow, nullptr );
			}
		}
	}

	void glfw_callback_button_( GLFWwindow* aWindow, int aBut, int aAct, int )
	{
		auto* state = static_cast<State*>(glfwGetWindowUserPointer( aWindow ));
		assert( state );

		if( EInputMode::piloting == state->inputMode )
		{
			if( GLFW_MOUSE_BUTTON_RIGHT == aBut )
			{
				if( GLFW_PRESS == aAct )
					state->thrustMouseHeld = true;
				else if( GLFW_RELEASE == aAct )
					state->thrustMouseHeld = false;
			}
			if( GLFW_MOUSE_BUTTON_LEFT == aBut && GLFW_PRESS == aAct && !state->showStartScreen && !state->countdownActive )
			{
				state->fireRequested = true;
			}
		}
	}

	void glfw_callback_motion_( GLFWwindow* aWindow, double aX, double aY )
	{
		auto* state = static_cast<State*>(glfwGetWindowUserPointer( aWindow ));
		assert( state );

		int iwidth, iheight;
		glfwGetFramebufferSize( aWindow, &iwidth, &iheight );

		if( EInputMode::piloting == state->inputMode )
		{
			Vec2f relative{ float(aX) - iwidth/2.f, iheight/2.f - float(aY) };
			state->player.angle = std::atan2( relative.y, relative.x );
		}
	}
}

namespace
{
	GLFWCleanupHelper::~GLFWCleanupHelper()
	{
		glfwTerminate();
	}

	GLFWWindowDeleter::~GLFWWindowDeleter()
	{
		if( window )
			glfwDestroyWindow( window );
	}
}

