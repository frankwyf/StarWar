#include <glad.h>
#include <GLFW/glfw3.h>

#include <random>
#include <typeinfo>
#include <stdexcept>

#include <vector>
#include <algorithm>
#include <string_view>
#include <string>
#include <cctype>
#include <cmath>
#include <thread>
#include <filesystem>
#include <fstream>

#include <cstdio>
#include <cstdlib>

#if defined(_WIN32)
#	if !defined(NOMINMAX)
#		define NOMINMAX
#	endif
#	if !defined(WIN32_LEAN_AND_MEAN)
#		define WIN32_LEAN_AND_MEAN
#	endif
#	include <windows.h>
#endif

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"
#include "../draw2d/shape.hpp"
#include "../draw2d/image.hpp"

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
	constexpr float kComboWindowSeconds = 3.5f;
	constexpr float kRapidFireDuration = 6.0f;
  constexpr float kOverdriveDuration = 5.0f;
	constexpr float kExhaustInterval = 0.025f;
	constexpr float kExhaustLife = 0.55f;
	constexpr float kExhaustSpeed = 160.f;
	constexpr float kScorePopupLife = 1.2f;
	constexpr float kMinimapSize = 130.f;
	constexpr float kMinimapMargin = 14.f;
	constexpr float kMinimapWorldRange = 1800.f;
	constexpr float kDangerIndicatorDist = 80.f;

	enum class EnemyArchetype
	{
		fighter,
		strafer,
		rusher
	};

	enum class PickupType
	{
		shield,
       rapidfire,
		overdrive
	};

	enum class AudioEvent
	{
		fire,
		hit,
		enemyDown,
		bossSpawn,
		pickup,
		playerDown
	};

	char const* safe_cstr_( char const* aText ) noexcept
	{
		return aText ? aText : "<no error description>";
	}

	std::filesystem::path project_root_guess_()
	{
		auto cwd = std::filesystem::current_path();
		for( int i = 0; i < 8; ++i )
		{
			if( std::filesystem::exists( cwd / "assets" ) )
				return cwd;
			if( !cwd.has_parent_path() )
				break;
			cwd = cwd.parent_path();
		}
		return std::filesystem::current_path();
	}

	std::filesystem::path profile_path_()
	{
		auto const root = project_root_guess_();
		return root / "save" / "profile.cfg";
	}

	void load_profile_( State& aState )
	{
		auto const p = profile_path_();
		if( !std::filesystem::exists( p ) )
			return;

		std::ifstream fin( p );
		if( !fin )
			return;

		std::string line;
		while( std::getline( fin, line ) )
		{
			auto const eq = line.find( '=' );
			if( std::string::npos == eq )
				continue;

			auto const key = line.substr( 0, eq );
			auto const value = line.substr( eq + 1 );

			if( "high_score" == key )
				aState.highScore = std::max( 0, std::atoi( value.c_str() ) );
			else if( "max_wave" == key )
				aState.maxWaveReached = std::max( 0, std::atoi( value.c_str() ) );
			else if( "audio_enabled" == key )
				aState.audioEnabled = 0 != std::atoi( value.c_str() );
			else if( "show_minimap" == key )
				aState.showMinimap = 0 != std::atoi( value.c_str() );
			else if( "difficulty" == key )
			{
				int const d = std::atoi( value.c_str() );
				aState.difficulty = d <= 0 ? EDifficulty::easy : (d >= 2 ? EDifficulty::hard : EDifficulty::normal);
			}
		}
	}

	struct GameplayTuning
	{
		float enemyHpWaveScale = 0.08f;
		float eliteBaseChance = 0.05f;
		float eliteWaveChanceScale = 0.01f;
		float eliteMaxChance = 0.22f;
		float playerBaseDamage = 1.f;
		float playerLv3Damage = 1.4f;
	};

	GameplayTuning load_gameplay_tuning_()
	{
		GameplayTuning tuning;
		auto const root = project_root_guess_();
		auto const path = root / "config" / "gameplay.cfg";
		if( !std::filesystem::exists( path ) )
			return tuning;

		std::ifstream fin( path );
		if( !fin )
			return tuning;

		auto read_float = [&]( std::string const& key, float& value, std::string const& line )
		{
			auto const eq = line.find( '=' );
			if( eq == std::string::npos )
				return;
			auto const k = line.substr( 0, eq );
			if( k != key )
				return;
			value = std::strtof( line.substr( eq + 1 ).c_str(), nullptr );
		};

		std::string line;
		while( std::getline( fin, line ) )
		{
			read_float( "enemy_hp_wave_scale", tuning.enemyHpWaveScale, line );
			read_float( "elite_base_chance", tuning.eliteBaseChance, line );
			read_float( "elite_wave_chance_scale", tuning.eliteWaveChanceScale, line );
			read_float( "elite_max_chance", tuning.eliteMaxChance, line );
			read_float( "player_base_damage", tuning.playerBaseDamage, line );
			read_float( "player_lv3_damage", tuning.playerLv3Damage, line );
		}

		return tuning;
	}

	void save_profile_( State const& aState )
	{
		auto const p = profile_path_();
		std::filesystem::create_directories( p.parent_path() );

		std::ofstream fout( p, std::ios::trunc );
		if( !fout )
			return;

		int const d = aState.difficulty == EDifficulty::easy ? 0 : (aState.difficulty == EDifficulty::hard ? 2 : 1);
		fout << "high_score=" << std::max( aState.highScore, aState.score ) << "\n";
		fout << "max_wave=" << std::max( aState.maxWaveReached, aState.wave ) << "\n";
		fout << "audio_enabled=" << (aState.audioEnabled ? 1 : 0) << "\n";
		fout << "show_minimap=" << (aState.showMinimap ? 1 : 0) << "\n";
		fout << "difficulty=" << d << "\n";
	}

	void save_surface_ppm_( Surface const& aSurface, std::filesystem::path const& aPath )
	{
		std::filesystem::create_directories( aPath.parent_path() );
		std::ofstream out( aPath, std::ios::binary | std::ios::trunc );
		if( !out )
			return;

		auto const w = aSurface.get_width();
		auto const h = aSurface.get_height();
		out << "P6\n" << w << " " << h << "\n255\n";

		auto const* ptr = aSurface.get_surface_ptr();
		for( std::uint32_t i = 0; i < w * h; ++i )
		{
			out.write( reinterpret_cast<char const*>(ptr + i * 4), 3 );
		}
	}

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
     bool elite = false;
		EnemyArchetype archetype = EnemyArchetype::fighter;
		float aiTimer = 0.f;
	};

	struct Pickup
	{
		Vec2f pos;
		Vec2f vel;
		float life = 9.f;
		PickupType type = PickupType::shield;
	};

	struct FxParticle
	{
		Vec2f pos;
		Vec2f vel;
		float life = 0.f;
		float maxLife = 0.f;
		ColorU8_sRGB color;
	};

	struct ScorePopup
	{
		Vec2f pos;
		int value = 0;
		float life = kScorePopupLife;
		ColorU8_sRGB color;
	};

	struct ExhaustTrail
	{
		Vec2f pos;
		float life = kExhaustLife;
		float maxLife = kExhaustLife;
		ColorU8_sRGB color;
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

	void play_audio_event_( AudioEvent aEvent, bool aEnabled )
	{
		if( !aEnabled )
			return;

#if defined(_WIN32)
		UINT beepType = MB_OK;
		switch( aEvent )
		{
		case AudioEvent::fire: beepType = MB_OK; break;
		case AudioEvent::hit: beepType = MB_ICONHAND; break;
		case AudioEvent::enemyDown: beepType = MB_ICONASTERISK; break;
		case AudioEvent::bossSpawn: beepType = MB_ICONEXCLAMATION; break;
		case AudioEvent::pickup: beepType = MB_ICONINFORMATION; break;
		case AudioEvent::playerDown: beepType = MB_ICONHAND; break;
		default: break;
		}
		std::thread( [beepType] { MessageBeep( beepType ); } ).detach();
#else
		(void)aEvent;
#endif
	}

	void spawn_burst_fx_( std::vector<FxParticle>& aFx, Vec2f aPos, ColorU8_sRGB aColor, int aCount, float aSpeed, RNG& aRng )
	{
		std::uniform_real_distribution<float> ang( 0.f, 6.2831853f );
		std::uniform_real_distribution<float> mag( 0.35f, 1.0f );
		for( int i = 0; i < aCount; ++i )
		{
			float const t = ang( aRng );
			float const m = mag( aRng ) * aSpeed;
			aFx.push_back( FxParticle{
				aPos,
				Vec2f{ std::cos( t ) * m, std::sin( t ) * m },
				0.45f,
				0.45f,
				aColor
			} );
		}
	}

	int combo_multiplier_( int comboCount )
	{
		return std::clamp( 1 + comboCount / 4, 1, 5 );
	}

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

	void glfw_callback_error_( int aErrNum, char const* aErrDesc )
	{
		std::fprintf( stderr, "GLFW error: %s (%d)\n", safe_cstr_(aErrDesc), aErrNum );
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

		if( GLFW_KEY_M == aKey && GLFW_PRESS == aAction )
		{
			state->audioEnabled = !state->audioEnabled;
			return;
		}

		if( GLFW_KEY_P == aKey && GLFW_PRESS == aAction )
		{
			if( !state->showStartScreen && !state->gameOver )
				state->paused = !state->paused;
			return;
		}

		if( GLFW_KEY_F12 == aKey && GLFW_PRESS == aAction )
		{
			state->screenshotRequested = true;
			return;
		}

		if( GLFW_KEY_F3 == aKey && GLFW_PRESS == aAction )
		{
			state->showDebugOverlay = !state->showDebugOverlay;
			return;
		}

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
				play_audio_event_( AudioEvent::bossSpawn, state->audioEnabled );
			}
			state->inputMode = EInputMode::piloting;
			glfwSetCursor( aWindow, state->crosshair );
			return;
		}

		if( GLFW_KEY_R == aKey && GLFW_PRESS == aAction )
		{
			state->restartRequested = true;
			play_audio_event_( AudioEvent::pickup, state->audioEnabled );
			return;
		}

		if( GLFW_KEY_TAB == aKey && GLFW_PRESS == aAction )
		{
			state->showMinimap = !state->showMinimap;
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

int main( int aArgc, char* aArgv[] ) try
{
	// Parse command line arguments
	RuntimeConfig const config = parse_command_line( aArgc, aArgv );

	if( config.selfTestAssets )
	{
		auto img = load_image( "assets/earth.png" );
		std::printf( "Asset self-test OK: earth.png (%ux%u)\n", img->get_width(), img->get_height() );
		return 0;
	}

	// Initialize GLFW
	if( GLFW_TRUE != glfwInit() )
	{
		char const* msg = nullptr;
		int ecode = glfwGetError( &msg );
		throw Error( "glfwInit() failed with '%s' (%d)", safe_cstr_(msg), ecode );
	}

	// Ensure that we call glfwTerminate() at the end of the program.
	GLFWCleanupHelper cleanupHelper;

	// Configure GLFW and create window
	glfwSetErrorCallback( &glfw_callback_error_ );

	glfwWindowHint( GLFW_SRGB_CAPABLE, GLFW_TRUE );
	glfwWindowHint( GLFW_DOUBLEBUFFER, GLFW_TRUE );

	//glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE ); // Allow resizing! Do not change this!

#	if defined(_WIN32)
	// Windows release compatibility: request OpenGL 3.3 core.
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 3 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
#	elif defined(__APPLE__)
	// Apple supports at most OpenGL 4.1.
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
#	else
	// Linux and other platforms can target OpenGL 4.3.
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
#	endif
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
		throw Error( "glfwCreateWindow() failed with '%s' (%d)", safe_cstr_(msg), ecode );
	}

	GLFWWindowDeleter windowDeleter{ window };

	// Runtime state
	State state;
    load_profile_( state );
    auto const gameplay = load_gameplay_tuning_();
	std::vector<Bullet> bullets;
	bullets.reserve( 256 );
	std::vector<Enemy> enemies;
	enemies.reserve( 16 );
	std::vector<Pickup> pickups;
	pickups.reserve( 16 );
	std::vector<FxParticle> fx;
	fx.reserve( 256 );
	std::vector<ScorePopup> scorePopups;
	scorePopups.reserve( 32 );
	std::vector<ExhaustTrail> exhaust;
	exhaust.reserve( 128 );
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
	unsigned const runtimeSeed = config.fixedSeedEnabled
		? config.fixedSeed
		: unsigned(std::random_device{}());
	RNG rng( runtimeSeed );

	Background background( rng, fbwidth, fbheight );
	AsteroidField asteroids( rng, fbwidth, fbheight );

	auto const spaceship = make_spaceship_shape();


	// Main loop
	auto lastUpdateTime = Clock::now();
	float smokeElapsed = 0.f;
	bool smokeCaptureDone = false;
	float smokeFireTimer = 0.f;
	bool const smokeAutomation = config.smokeTestSeconds > 0.f;

	if( smokeAutomation )
	{
		state.showStartScreen = false;
		state.gameStarted = true;
		state.countdownActive = false;
		state.inputMode = EInputMode::piloting;
		state.lives = std::max( 1, 3 + difficulty_extra_start_lives_( state.difficulty ) );
		state.shieldMax = difficulty_start_shield_( state.difficulty );
		state.shield = state.shieldMax;
		state.shieldRegenRate = difficulty_shield_regen_rate_( state.difficulty );
	}

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
     auto const frameDt = std::chrono::duration_cast<Secondsf>(now - lastUpdateTime).count();
		lastUpdateTime = now;
		auto const dt = state.paused ? 0.f : frameDt;
		smokeElapsed += frameDt;

		if( smokeAutomation )
		{
			state.showStartScreen = false;
			state.gameStarted = true;
			state.countdownActive = false;
			state.paused = false;

			state.player.angle = std::sin( smokeElapsed * 0.9f ) * 2.5f;
			state.thrustKeyHeld = std::sin( smokeElapsed * 1.3f ) > -0.15f;
			smokeFireTimer -= frameDt;
			if( smokeFireTimer <= 0.f )
			{
				state.fireRequested = true;
				smokeFireTimer = 0.075f;
			}
		}

		if( state.restartRequested )
		{
         save_profile_( state );
			auto* cursor = state.crosshair;
			auto const selectedDifficulty = state.difficulty;
			auto const audioEnabled = state.audioEnabled;
			auto const bestScore = std::max( state.highScore, state.score );
			auto const bestWave = std::max( state.maxWaveReached, state.wave );
			auto const minimap = state.showMinimap;
			state = State{};
			state.crosshair = cursor;
			state.difficulty = selectedDifficulty;
			state.audioEnabled = audioEnabled;
			state.highScore = bestScore;
			state.maxWaveReached = bestWave;
			state.showMinimap = minimap;
			state.inputMode = EInputMode::piloting;
			state.showStartScreen = true;
			glfwSetCursor( window, nullptr );
			bullets.clear();
			enemies.clear();
			pickups.clear();
			fx.clear();
			scorePopups.clear();
			exhaust.clear();
		}

		state_update( state, dt );

		if( state.countdownActive && state.countdownTime <= 0.f )
		{
			state.countdownActive = false;
		}

       if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive && state.fireRequested && state.fireCooldown <= 0.f )
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
			if( state.weaponLevel >= 4 )
			{
				// Level 4: extra wide-angle shots + rear shot
				Mat22f wl = make_rotation_2d( 0.32f );
				Mat22f wr = make_rotation_2d( -0.32f );
				bullets.push_back( Bullet{ muzzle, (wl * dir) * (kBulletSpeed * 0.75f), kBulletLife * 0.8f, false } );
				bullets.push_back( Bullet{ muzzle, (wr * dir) * (kBulletSpeed * 0.75f), kBulletLife * 0.8f, false } );
				bullets.push_back( Bullet{ muzzle - dir * 30.f, (-dir) * (kBulletSpeed * 0.5f), kBulletLife * 0.6f, false } );
			}
			state.fireCooldown = state.weaponLevel >= 4 ? 0.07f : (state.weaponLevel >= 3 ? 0.09f : 0.18f);
			if( state.rapidFireTime > 0.f )
				state.fireCooldown *= 0.45f;
            if( state.overdriveTime > 0.f )
				state.fireCooldown *= 0.68f;
			spawn_burst_fx_( fx, muzzle, ColorU8_sRGB{ 255, 220, 120 }, 3, 120.f, rng );
			play_audio_event_( AudioEvent::fire, state.audioEnabled );
		}
		state.fireRequested = false;

		// Engine exhaust trail
		if( !state.showStartScreen && !state.gameOver && state.player.accelerationMagnitude > 0.f && state.exhaustTimer >= kExhaustInterval )
		{
			state.exhaustTimer = 0.f;
			Vec2f dir{ std::cos( state.player.angle ), std::sin( state.player.angle ) };
			Vec2f exhaustPos = Vec2f{ fbwidth*0.5f, fbheight*0.5f } - dir * 22.f;
			std::uniform_real_distribution<float> jitter( -4.f, 4.f );
			exhaust.push_back( ExhaustTrail{ { exhaustPos.x + jitter(rng), exhaustPos.y + jitter(rng) }, kExhaustLife, kExhaustLife, ColorU8_sRGB{ 100, 160, 255 } } );
			exhaust.push_back( ExhaustTrail{ { exhaustPos.x + jitter(rng), exhaustPos.y + jitter(rng) }, kExhaustLife * 0.7f, kExhaustLife * 0.7f, ColorU8_sRGB{ 200, 220, 255 } } );
		}

        if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive && enemies.empty() )
		{
         auto enemySpeedScale = difficulty_enemy_speed_scale_( state.difficulty );
			auto enemyFireScale = difficulty_enemy_fire_scale_( state.difficulty );
			if( state.surgeTime > 0.f )
			{
				enemySpeedScale *= 1.18f;
				enemyFireScale *= 0.82f;
			}
			float const waveScale = 1.f + state.wave * 0.04f; // progressive difficulty per wave
			if( state.wave % 4 == 0 )
			{
				Enemy boss;
				boss.boss = true;
				boss.hp = 14.f + state.wave * 1.5f;
				boss.pos = { fbwidth + 160.f, fbheight * 0.5f };
				boss.vel = { -90.f * enemySpeedScale * waveScale, 0.f };
				boss.fireCooldown = 1.0f * enemyFireScale;
				enemies.push_back( boss );
				state.bossSpawned = true;
				state.waveBannerTime = 2.2f;
				state.screenShakeTime = std::max( state.screenShakeTime, 0.35f );
				state.screenShakeStrength = std::max( state.screenShakeStrength, 6.f );
				play_audio_event_( AudioEvent::bossSpawn, state.audioEnabled );
			}
			else
			{
				int count = std::max( 1, 2 + state.wave + difficulty_wave_enemy_bonus_( state.difficulty ) );
				count = std::min( count, 12 ); // cap enemies per wave
				std::uniform_real_distribution<float> sideY( 100.f, std::max( 120.f, float(fbheight) - 100.f ) );
				for( int i = 0; i < count; ++i )
				{
					Enemy e;
					e.pos = { (i % 2 == 0 ? -40.f : fbwidth + 40.f), sideY(rng) };
					e.vel = { ((i % 2 == 0 ? 100.f : -100.f) * (1.f + 0.06f * state.wave)) * enemySpeedScale, ((unit01(rng) - 0.5f) * 30.f) * enemySpeedScale };
					e.fireCooldown = (0.5f + unit01(rng)) * enemyFireScale;
                 e.hp = 1.f + state.wave * gameplay.enemyHpWaveScale; // enemies get tougher
					float const roll = unit01( rng );
					if( roll < 0.22f )
					{
						e.archetype = EnemyArchetype::rusher;
						e.hp *= 1.2f;
					}
					else if( roll < 0.52f )
					{
						e.archetype = EnemyArchetype::strafer;
						e.fireCooldown *= 0.85f;
					}

					if( state.wave >= 3 )
					{
                     float const eliteChance = std::clamp(
							gameplay.eliteBaseChance + state.wave * gameplay.eliteWaveChanceScale,
							0.f,
							gameplay.eliteMaxChance
						);
						if( unit01(rng) < eliteChance )
						{
							e.elite = true;
							e.hp *= 1.65f;
							e.fireCooldown *= 0.75f;
							e.vel *= 1.12f;
						}
					}
					enemies.push_back( e );
				}
			}
		}

        if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive && enemies.empty() )
		{
			++state.wave;
			if( state.wave >= 2 ) state.weaponLevel = 2;
			if( state.wave >= 4 ) state.weaponLevel = 3;
			if( state.wave >= 7 ) state.weaponLevel = 4;
            if( state.wave % 5 == 0 )
				state.surgeTime = 8.f;
			state.shield = std::min( state.shieldMax, state.shield + 22.f );
			state.waveBannerTime = 2.0f;
			state.screenShakeTime = std::max( state.screenShakeTime, 0.08f );
			state.screenShakeStrength = std::max( state.screenShakeStrength, 1.8f );
		}

       if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive )
		{
         auto enemySpeedScale = difficulty_enemy_speed_scale_( state.difficulty );
			auto enemyFireScale = difficulty_enemy_fire_scale_( state.difficulty );
			if( state.surgeTime > 0.f )
			{
				enemySpeedScale *= 1.18f;
				enemyFireScale *= 0.82f;
			}
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
				e.aiTimer += state.thisFrame.dt;
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
					if( e.archetype == EnemyArchetype::strafer )
					{
						Vec2f perp{ -toPlayer.y, toPlayer.x };
						e.vel += perp * (120.f * state.thisFrame.dt * enemySpeedScale);
					}
					else if( e.archetype == EnemyArchetype::rusher )
					{
						if( e.aiTimer > 1.4f )
						{
							e.vel += toPlayer * (200.f * enemySpeedScale);
							e.aiTimer = 0.f;
						}
					}

					e.vel.x = std::clamp( e.vel.x, -260.f * enemySpeedScale, 260.f * enemySpeedScale );
					e.vel.y = std::clamp( e.vel.y, -260.f * enemySpeedScale, 260.f * enemySpeedScale );
					if( e.elite )
					{
						e.vel.x = std::clamp( e.vel.x, -320.f * enemySpeedScale, 320.f * enemySpeedScale );
						e.vel.y = std::clamp( e.vel.y, -320.f * enemySpeedScale, 320.f * enemySpeedScale );
					}

					e.fireCooldown -= state.thisFrame.dt;
					if( e.fireCooldown <= 0.f )
					{
						float jitter = (unit01(rng) - 0.5f) * 0.30f;
						Mat22f miss = make_rotation_2d( jitter );
                        float const eliteBulletScale = e.elite ? 1.2f : 1.f;
						bullets.push_back( Bullet{ e.pos, (miss * toPlayer) * (kEnemyBulletSpeed * 0.95f * enemySpeedScale * eliteBulletScale), 1.9f, true } );
						if( e.archetype == EnemyArchetype::strafer )
						{
							Mat22f side = make_rotation_2d( 0.18f );
							bullets.push_back( Bullet{ e.pos, (side * (miss * toPlayer)) * (kEnemyBulletSpeed * 0.82f * enemySpeedScale), 1.7f, true } );
						}
						e.fireCooldown = (1.35f + unit01(rng) * 0.7f) * enemyFireScale;
						if( e.archetype == EnemyArchetype::rusher )
							e.fireCooldown *= 1.25f;
                       if( e.elite )
							e.fireCooldown *= 0.78f;
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
					state.score += int(hits) * 10 * combo_multiplier_( state.comboCount );
					spawn_burst_fx_( fx, it->pos, ColorU8_sRGB{ 180, 180, 180 }, int(hits) * 2, 150.f, rng );
					consumed = true;
				}
				for( auto eit = enemies.begin(); eit != enemies.end() && !consumed; )
				{
					float hitR = eit->boss ? 46.f : 20.f;
					auto d = eit->pos - it->pos;
					if( dot(d,d) <= hitR*hitR )
					{
                     float dmg = state.weaponLevel >= 3 ? gameplay.playerLv3Damage : gameplay.playerBaseDamage;
						if( state.overdriveTime > 0.f )
							dmg *= 1.4f;
						eit->hp -= dmg;
						consumed = true;
						spawn_burst_fx_( fx, it->pos, ColorU8_sRGB{ 255, 140, 90 }, eit->boss ? 7 : 4, eit->boss ? 180.f : 120.f, rng );
						if( eit->hp <= 0.f )
							{
								++state.comboCount;
								state.comboTimer = kComboWindowSeconds;
                           int const baseKillScore = eit->boss ? 300 : (eit->elite ? 90 : 40);
							int const killScore = baseKillScore * combo_multiplier_( state.comboCount );
								state.score += killScore;
								++state.totalKills;
                         if( eit->elite ) ++state.eliteKills;
								if( eit->boss ) ++state.bossesDefeated;
                         scorePopups.push_back( ScorePopup{ eit->pos, killScore, kScorePopupLife, eit->boss ? ColorU8_sRGB{ 255, 100, 100 } : (eit->elite ? ColorU8_sRGB{ 140, 255, 210 } : ColorU8_sRGB{ 255, 230, 100 }) } );
                         if( !eit->boss && unit01(rng) < 0.30f )
								{
                              PickupType ptype = PickupType::shield;
								if( eit->elite )
									ptype = unit01(rng) < 0.55f ? PickupType::overdrive : PickupType::rapidfire;
								else
									ptype = unit01(rng) < 0.55f ? PickupType::shield : PickupType::rapidfire;

									pickups.push_back( Pickup{
										eit->pos,
										Vec2f{ (unit01(rng)-0.5f) * 90.f, (unit01(rng)-0.5f) * 90.f },
										9.f,
                                    ptype
									} );
								}
								play_audio_event_( AudioEvent::enemyDown, state.audioEnabled );
								state.screenShakeTime = std::max( state.screenShakeTime, eit->boss ? 0.35f : 0.12f );
								state.screenShakeStrength = std::max( state.screenShakeStrength, eit->boss ? 9.f : 3.f );
								spawn_burst_fx_( fx, eit->pos, ColorU8_sRGB{ 255, 200, 80 }, eit->boss ? 18 : 9, eit->boss ? 240.f : 160.f, rng );
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
						state.hitFlashTime = 0.12f;
						state.screenShakeTime = std::max( state.screenShakeTime, 0.1f );
						state.screenShakeStrength = std::max( state.screenShakeStrength, 2.8f );
						play_audio_event_( AudioEvent::hit, state.audioEnabled );
					}
					else
					{
						--state.lives;
						state.invulnerabilityTime = kRespawnInvulnerability;
						state.player.velocity = { 0.f, 0.f };
						state.hitFlashTime = 0.18f;
						state.screenShakeTime = std::max( state.screenShakeTime, 0.2f );
						state.screenShakeStrength = std::max( state.screenShakeStrength, 4.8f );
						play_audio_event_( AudioEvent::playerDown, state.audioEnabled );
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

		for( auto pit = pickups.begin(); pit != pickups.end(); )
		{
			pit->life -= state.thisFrame.dt;
			pit->pos += pit->vel * state.thisFrame.dt - state.thisFrame.movement;
			pit->vel *= 0.98f;

			// Pickup magnet: attract toward player when within range
           if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive )
			{
				Vec2f const playerScreen{ fbwidth * 0.5f, fbheight * 0.5f };
				Vec2f const toPlayer = playerScreen - pit->pos;
				float const distSq = dot( toPlayer, toPlayer );
				float const magnetRange = 120.f;
				if( distSq < magnetRange * magnetRange && distSq > 1.f )
				{
					float const dist = std::sqrt( distSq );
					float const magnetForce = 280.f * (1.f - dist / magnetRange);
					pit->vel += (toPlayer / dist) * (magnetForce * state.thisFrame.dt);
				}
			}

			bool consumed = false;
           if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive )
			{
				auto const d = pit->pos - Vec2f{ fbwidth * 0.5f, fbheight * 0.5f };
				if( dot( d, d ) <= 22.f * 22.f )
				{
					if( pit->type == PickupType::shield )
					{
						state.shield = std::min( state.shieldMax, state.shield + 26.f );
					}
                    else if( pit->type == PickupType::rapidfire )
					{
						state.rapidFireTime = std::max( state.rapidFireTime, kRapidFireDuration );
					}
                  else
					{
						state.overdriveTime = std::max( state.overdriveTime, kOverdriveDuration );
						state.shield = std::min( state.shieldMax, state.shield + 10.f );
					}
					state.waveBannerTime = std::max( state.waveBannerTime, 1.0f );
					play_audio_event_( AudioEvent::pickup, state.audioEnabled );
					spawn_burst_fx_( fx, pit->pos, ColorU8_sRGB{ 130, 230, 255 }, 8, 130.f, rng );
					consumed = true;
				}
			}

			if( pit->life <= 0.f || consumed )
				pit = pickups.erase( pit );
			else
				++pit;
		}

		for( auto fit = fx.begin(); fit != fx.end(); )
		{
			fit->life -= state.thisFrame.dt;
			fit->pos += fit->vel * state.thisFrame.dt - state.thisFrame.movement;
			fit->vel *= 0.93f;
			if( fit->life <= 0.f )
				fit = fx.erase( fit );
			else
				++fit;
		}

		if( fx.size() > 1200 )
			fx.erase( fx.begin(), fx.begin() + (fx.size() - 1200) );

		if( pickups.size() > 120 )
			pickups.erase( pickups.begin(), pickups.begin() + (pickups.size() - 120) );

		// Update exhaust trails
		for( auto eit = exhaust.begin(); eit != exhaust.end(); )
		{
			eit->life -= state.thisFrame.dt;
			eit->pos -= state.thisFrame.movement;
			if( eit->life <= 0.f )
				eit = exhaust.erase( eit );
			else
				++eit;
		}
		if( exhaust.size() > 300 )
			exhaust.erase( exhaust.begin(), exhaust.begin() + (exhaust.size() - 300) );

		// Update score popups
		for( auto sit = scorePopups.begin(); sit != scorePopups.end(); )
		{
			sit->life -= state.thisFrame.dt;
			sit->pos.y += 40.f * state.thisFrame.dt;
			sit->pos -= state.thisFrame.movement;
			if( sit->life <= 0.f )
				sit = scorePopups.erase( sit );
			else
				++sit;
		}

		// Update high score
		state.highScore = std::max( state.highScore, state.score );
		state.maxWaveReached = std::max( state.maxWaveReached, state.wave );

     if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive && state.shieldRegenCooldown <= 0.f && state.shield < state.shieldMax )
		{
			state.shield = std::min( state.shieldMax, state.shield + state.shieldRegenRate * state.thisFrame.dt );
		}

		background.update( state.player.position, state.thisFrame.movement );
		asteroids.update( state.thisFrame.dt, state.thisFrame.movement );

       if( !state.paused && !state.showStartScreen && !state.gameOver && !state.countdownActive && state.invulnerabilityTime <= 0.f )
		{
			auto absorb_or_hit_life = [&]( float shieldDamage )
			{
				state.comboCount = 0;
				state.comboTimer = 0.f;
				state.hitFlashTime = 0.15f;
				if( state.shield > 0.f )
				{
					state.shield = std::max( 0.f, state.shield - shieldDamage );
					state.shieldRegenCooldown = 2.0f;
					state.invulnerabilityTime = 0.24f;
					state.screenShakeTime = std::max( state.screenShakeTime, 0.12f );
					state.screenShakeStrength = std::max( state.screenShakeStrength, 3.5f );
					play_audio_event_( AudioEvent::hit, state.audioEnabled );
				}
				else
				{
					--state.lives;
					state.invulnerabilityTime = kRespawnInvulnerability;
					state.player.velocity = { 0.f, 0.f };
					state.screenShakeTime = std::max( state.screenShakeTime, 0.2f );
					state.screenShakeStrength = std::max( state.screenShakeStrength, 5.f );
					play_audio_event_( AudioEvent::playerDown, state.audioEnabled );
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

		Vec2f renderShake{ 0.f, 0.f };
		if( state.screenShakeTime > 0.f && state.screenShakeStrength > 0.f )
		{
			std::uniform_real_distribution<float> shakeDist( -state.screenShakeStrength, state.screenShakeStrength );
			renderShake = { shakeDist( rng ), shakeDist( rng ) };
		}

		// Draw scene
		surface.clear();
		background.draw( surface );
		asteroids.draw( surface );

		// Draw exhaust trails (behind everything)
		for( auto const& et : exhaust )
		{
			float const alpha = et.life / et.maxLife;
			float const sz = 1.5f + (1.f - alpha) * 2.f;
			std::uint8_t const r = static_cast<std::uint8_t>( et.color.r * alpha );
			std::uint8_t const g = static_cast<std::uint8_t>( et.color.g * alpha );
			std::uint8_t const b = static_cast<std::uint8_t>( et.color.b * alpha );
			draw_rectangle_solid( surface, et.pos + renderShake - Vec2f{ sz, sz }, et.pos + renderShake + Vec2f{ sz, sz }, ColorU8_sRGB{ r, g, b } );
		}

		for( auto const& e : enemies )
		{
			auto const er = make_rotation_2d( e.angle );
			ColorF ec = e.boss ? ColorF{ 0.8f, 0.2f, 0.2f } : ColorF{ 0.8f, 0.5f, 0.2f };
         if( e.elite && !e.boss )
				ec = ColorF{ 0.28f, 0.9f, 0.7f };
			if( !e.boss && e.archetype == EnemyArchetype::strafer )
				ec = ColorF{ 0.85f, 0.65f, 0.25f };
			if( !e.boss && e.archetype == EnemyArchetype::rusher )
				ec = ColorF{ 0.9f, 0.38f, 0.3f };
         if( e.elite && e.archetype == EnemyArchetype::strafer )
				ec = ColorF{ 0.28f, 0.82f, 0.72f };
			if( e.elite && e.archetype == EnemyArchetype::rusher )
				ec = ColorF{ 0.26f, 0.76f, 0.64f };
			spaceship.draw( surface, ec, er, e.pos + renderShake );
			if( e.boss )
			{
				draw_rectangle_outline( surface, e.pos + renderShake + Vec2f{-40.f,-46.f}, e.pos + renderShake + Vec2f{40.f,-40.f}, ColorU8_sRGB{200,200,200} );
				draw_rectangle_solid( surface, e.pos + renderShake + Vec2f{-38.f,-44.f}, e.pos + renderShake + Vec2f{-38.f + 76.f * (e.hp/(14.f + state.wave)), -42.f}, ColorU8_sRGB{220,80,80} );
			}
		}

		for( auto const& bullet : bullets )
		{
			float const sz = bullet.enemy ? 2.5f : 3.f;
			ColorU8_sRGB const bc = bullet.enemy ? ColorU8_sRGB{ 255, 90, 90 } : ColorU8_sRGB{ 255, 230, 120 };
			draw_rectangle_solid(
				surface,
				bullet.pos + renderShake - Vec2f{ sz, sz },
				bullet.pos + renderShake + Vec2f{ sz, sz },
				bc
			);
			// Bullet glow halo
			ColorU8_sRGB const halo = bullet.enemy ? ColorU8_sRGB{ 160, 50, 50 } : ColorU8_sRGB{ 160, 140, 60 };
			draw_rectangle_solid(
				surface,
				bullet.pos + renderShake - Vec2f{ sz + 1.f, sz + 1.f },
				bullet.pos + renderShake - Vec2f{ sz, sz },
				halo
			);
		}

		for( auto const& p : pickups )
		{
           ColorU8_sRGB c = p.type == PickupType::shield
				? ColorU8_sRGB{ 90, 190, 255 }
				: (p.type == PickupType::rapidfire ? ColorU8_sRGB{ 255, 220, 110 } : ColorU8_sRGB{ 110, 255, 210 });
			draw_rectangle_outline( surface, p.pos + renderShake - Vec2f{ 9.f, 9.f }, p.pos + renderShake + Vec2f{ 9.f, 9.f }, c );
			draw_rectangle_solid( surface, p.pos + renderShake - Vec2f{ 5.f, 5.f }, p.pos + renderShake + Vec2f{ 5.f, 5.f }, c );
			// Pulsing inner
			float pulse = std::abs( std::sin( state.totalPlayTime * 6.f ) );
			float inner = 2.f + pulse * 3.f;
			draw_rectangle_solid( surface, p.pos + renderShake - Vec2f{ inner, inner }, p.pos + renderShake + Vec2f{ inner, inner }, ColorU8_sRGB{ 255, 255, 255 } );
		}

		for( auto const& p : fx )
		{
			float const alpha = p.life / p.maxLife;
			float const sz = alpha * 2.f;
			std::uint8_t const r = static_cast<std::uint8_t>( p.color.r * alpha );
			std::uint8_t const g = static_cast<std::uint8_t>( p.color.g * alpha );
			std::uint8_t const b = static_cast<std::uint8_t>( p.color.b * alpha );
			draw_rectangle_solid( surface, p.pos + renderShake - Vec2f{ sz, sz }, p.pos + renderShake + Vec2f{ sz, sz }, ColorU8_sRGB{ r, g, b } );
		}

		// Score popups
		for( auto const& sp : scorePopups )
		{
			float const alpha = sp.life / kScorePopupLife;
			int scale = sp.value >= 200 ? 2 : 1;
			std::uint8_t const r = static_cast<std::uint8_t>( sp.color.r * alpha );
			std::uint8_t const g = static_cast<std::uint8_t>( sp.color.g * alpha );
			std::uint8_t const b = static_cast<std::uint8_t>( sp.color.b * alpha );
			char popText[32] = {};
			std::snprintf( popText, sizeof(popText), "%d", sp.value );
			draw_text_5x7_( surface, sp.pos + renderShake, scale, ColorU8_sRGB{ r, g, b }, popText );
		}

		auto const rot = make_rotation_2d( state.player.angle );
		auto const offs = Vec2f{ fbwidth*0.5f, fbheight*0.5f } + renderShake;
		auto const shipColor = state.invulnerabilityTime > 0.f
			? ColorF{ 0.9f, 0.8f, 0.2f }
			: ColorF{ 0.2f, 0.4f, 0.7f };
		spaceship.draw( surface, shipColor, rot, offs );

		// Shield glow ring around player when active
		if( state.shield > 0.f && !state.showStartScreen && !state.gameOver )
		{
			float shieldAlpha = std::clamp( state.shield / state.shieldMax, 0.1f, 1.f );
			std::uint8_t const sv = static_cast<std::uint8_t>( 80.f * shieldAlpha );
			float const shRad = 26.f + shieldAlpha * 4.f;
			draw_rectangle_outline( surface, offs - Vec2f{ shRad, shRad }, offs + Vec2f{ shRad, shRad }, ColorU8_sRGB{ sv, static_cast<std::uint8_t>(sv + 60), 255 } );
		}

		float const scoreBar = std::min( 1.f, state.score / 2000.f );
		draw_rectangle_solid( surface, { 18.f, 18.f }, { 222.f, 38.f }, ColorU8_sRGB{ 10, 14, 26 } );
		draw_rectangle_outline( surface, { 18.f, 18.f }, { 222.f, 38.f }, ColorU8_sRGB{ 180, 180, 180 } );
		draw_rectangle_solid( surface, { 20.f, 20.f }, { 20.f + 200.f * scoreBar, 36.f }, ColorU8_sRGB{ 90, 200, 120 } );
		{
			char scoreStr[64] = {};
			std::snprintf( scoreStr, sizeof(scoreStr), "%d", state.score );
			draw_text_5x7_( surface, { 226.f, 22.f }, 2, ColorU8_sRGB{ 200, 255, 180 }, scoreStr );
		}

		float const shieldRatio = state.shieldMax > 0.f ? std::clamp( state.shield / state.shieldMax, 0.f, 1.f ) : 0.f;
		draw_rectangle_solid( surface, { 18.f, 42.f }, { 222.f, 60.f }, ColorU8_sRGB{ 10, 14, 26 } );
		draw_rectangle_outline( surface, { 18.f, 42.f }, { 222.f, 60.f }, ColorU8_sRGB{ 120, 170, 255 } );
		draw_rectangle_solid( surface, { 20.f, 44.f }, { 20.f + 200.f * shieldRatio, 58.f }, ColorU8_sRGB{ 70, 140, 240 } );
		draw_text_5x7_( surface, { 226.f, 45.f }, 1, ColorU8_sRGB{ 160, 200, 255 }, "SHIELD" );

		// High score display
		if( state.highScore > 0 )
		{
			char hiText[64] = {};
			std::snprintf( hiText, sizeof(hiText), "HI %d", state.highScore );
			draw_text_5x7_( surface, { 20.f, float(fbheight) - 20.f }, 1, ColorU8_sRGB{ 180, 180, 100 }, hiText );
		}

		// FPS display
		{
			char fpsText[32] = {};
			std::snprintf( fpsText, sizeof(fpsText), "%.0f FPS", state.displayFps );
			draw_text_5x7_( surface, { float(fbwidth) - 80.f, float(fbheight) - 20.f }, 1, ColorU8_sRGB{ 120, 120, 120 }, fpsText );
		}

		if( state.comboCount > 1 && state.comboTimer > 0.f )
		{
			char comboText[64] = {};
			std::snprintf( comboText, sizeof(comboText), "COMBO x%d", combo_multiplier_( state.comboCount ) );
			draw_text_5x7_( surface, { 20.f, 66.f }, 2, ColorU8_sRGB{ 255, 210, 110 }, comboText );
		}
		if( state.rapidFireTime > 0.f )
		{
			char rapidText[64] = {};
			std::snprintf( rapidText, sizeof(rapidText), "RAPID FIRE %.1fs", state.rapidFireTime );
			draw_text_5x7_( surface, { 20.f, 88.f }, 2, ColorU8_sRGB{ 255, 240, 140 }, rapidText );
		}
		if( state.overdriveTime > 0.f )
		{
			char overText[64] = {};
			std::snprintf( overText, sizeof(overText), "OVERDRIVE %.1fs", state.overdriveTime );
			draw_text_5x7_( surface, { 20.f, 110.f }, 2, ColorU8_sRGB{ 120, 255, 210 }, overText );
		}
		if( state.surgeTime > 0.f )
		{
			char surgeText[64] = {};
			std::snprintf( surgeText, sizeof(surgeText), "WAVE SURGE %.1fs", state.surgeTime );
			draw_text_5x7_( surface, { 20.f, 132.f }, 2, ColorU8_sRGB{ 255, 170, 120 }, surgeText );
		}

		// Wave and kills info
		if( !state.showStartScreen && !state.countdownActive )
		{
			char infoText[96] = {};
         std::snprintf( infoText, sizeof(infoText), "WAVE %d  KILLS %d  ELITE %d", state.wave, state.totalKills, state.eliteKills );
			draw_text_5x7_( surface, { 20.f, float(fbheight) - 36.f }, 1, ColorU8_sRGB{ 160, 170, 200 }, infoText );
		}

		if( state.lives > 0 )
		{
			for( int i = 0; i < state.lives; ++i )
			{
				float x0 = float(fbwidth) - 22.f - i * 22.f;
				draw_rectangle_solid( surface, { x0 - 14.f, 20.f }, { x0, 36.f }, ColorU8_sRGB{ 220, 80, 80 } );
				draw_rectangle_outline( surface, { x0 - 14.f, 20.f }, { x0, 36.f }, ColorU8_sRGB{ 255, 140, 140 } );
			}
		}

		// Minimap
		if( state.showMinimap && !state.showStartScreen && !state.gameOver )
		{
			float const mmX = float(fbwidth) - kMinimapSize - kMinimapMargin;
			float const mmY = float(fbheight) - kMinimapSize - kMinimapMargin;
			draw_rectangle_solid( surface, { mmX, mmY }, { mmX + kMinimapSize, mmY + kMinimapSize }, ColorU8_sRGB{ 8, 12, 22 } );
			draw_rectangle_outline( surface, { mmX, mmY }, { mmX + kMinimapSize, mmY + kMinimapSize }, ColorU8_sRGB{ 60, 80, 120 } );

			Vec2f const mmCenter{ mmX + kMinimapSize * 0.5f, mmY + kMinimapSize * 0.5f };
			Vec2f const playerScreen{ fbwidth * 0.5f, fbheight * 0.5f };

			// Player blip (center)
			draw_rectangle_solid( surface, mmCenter - Vec2f{ 2.f, 2.f }, mmCenter + Vec2f{ 2.f, 2.f }, ColorU8_sRGB{ 100, 180, 255 } );

			// Enemy blips
			for( auto const& e : enemies )
			{
				Vec2f rel = (e.pos - playerScreen) * (kMinimapSize * 0.45f / kMinimapWorldRange);
				Vec2f mmPos = mmCenter + Vec2f{ rel.x, -rel.y };
				if( mmPos.x > mmX + 3.f && mmPos.x < mmX + kMinimapSize - 3.f && mmPos.y > mmY + 3.f && mmPos.y < mmY + kMinimapSize - 3.f )
				{
                  ColorU8_sRGB ec = e.boss ? ColorU8_sRGB{ 255, 60, 60 } : (e.elite ? ColorU8_sRGB{ 100, 255, 220 } : ColorU8_sRGB{ 255, 160, 60 });
					float esz = e.boss ? 3.f : 1.5f;
					draw_rectangle_solid( surface, mmPos - Vec2f{ esz, esz }, mmPos + Vec2f{ esz, esz }, ec );
				}
			}

			// Pickup blips
			for( auto const& p : pickups )
			{
				Vec2f rel = (p.pos - playerScreen) * (kMinimapSize * 0.45f / kMinimapWorldRange);
				Vec2f mmPos = mmCenter + Vec2f{ rel.x, -rel.y };
				if( mmPos.x > mmX + 3.f && mmPos.x < mmX + kMinimapSize - 3.f && mmPos.y > mmY + 3.f && mmPos.y < mmY + kMinimapSize - 3.f )
				{
					draw_rectangle_solid( surface, mmPos - Vec2f{ 1.f, 1.f }, mmPos + Vec2f{ 1.f, 1.f }, ColorU8_sRGB{ 90, 230, 160 } );
				}
			}
		}

		// Edge danger indicators for off-screen enemies
		if( !state.showStartScreen && !state.gameOver && !state.countdownActive )
		{
			Vec2f const playerScreen{ fbwidth * 0.5f, fbheight * 0.5f };
			for( auto const& e : enemies )
			{
				bool offscreen = e.pos.x < -10.f || e.pos.x > float(fbwidth) + 10.f || e.pos.y < -10.f || e.pos.y > float(fbheight) + 10.f;
				if( offscreen )
				{
					Vec2f dir = e.pos - playerScreen;
					float dist = length( dir );
					if( dist > 1.f ) dir = dir / dist;
					Vec2f indicator = playerScreen + dir * kDangerIndicatorDist;
					indicator.x = std::clamp( indicator.x, 12.f, float(fbwidth) - 12.f );
					indicator.y = std::clamp( indicator.y, 12.f, float(fbheight) - 12.f );
					ColorU8_sRGB dc = e.boss ? ColorU8_sRGB{ 255, 50, 50 } : ColorU8_sRGB{ 255, 160, 60 };
					draw_rectangle_solid( surface, indicator - Vec2f{ 4.f, 4.f }, indicator + Vec2f{ 4.f, 4.f }, dc );
					draw_rectangle_outline( surface, indicator - Vec2f{ 6.f, 6.f }, indicator + Vec2f{ 6.f, 6.f }, dc );
				}
			}
		}

		if( state.showDebugOverlay )
		{
			float const panelW = 280.f;
			float const panelH = 110.f;
			Vec2f const p0{ float(fbwidth) - panelW - 14.f, 80.f };
			Vec2f const p1{ float(fbwidth) - 14.f, 80.f + panelH };
			draw_rectangle_solid( surface, p0, p1, ColorU8_sRGB{ 10, 18, 34 } );
			draw_rectangle_outline( surface, p0, p1, ColorU8_sRGB{ 90, 130, 180 } );

			char perfText[128] = {};
			std::snprintf( perfText, sizeof(perfText), "FPS %.1f  FRAME %.2fms", state.displayFps, state.frameTimeMs );
			draw_text_5x7_( surface, { p0.x + 8.f, p1.y - 18.f }, 1, ColorU8_sRGB{ 190, 220, 255 }, perfText );

			float const gx0 = p0.x + 8.f;
			float const gy0 = p0.y + 10.f;
			float const gw = panelW - 16.f;
			float const gh = panelH - 36.f;
			draw_rectangle_outline( surface, { gx0, gy0 }, { gx0 + gw, gy0 + gh }, ColorU8_sRGB{ 70, 95, 130 } );

			for( std::size_t i = 0; i < state.frameTimeHistory.size(); ++i )
			{
				std::size_t idx = (state.frameTimeHistoryIndex + i) % state.frameTimeHistory.size();
				float ms = state.frameTimeHistory[idx];
				float yNorm = std::clamp( ms / 33.3f, 0.f, 1.f );
				float x = gx0 + (float(i) / float(state.frameTimeHistory.size() - 1)) * gw;
				float y = gy0 + yNorm * gh;
				draw_rectangle_solid( surface, { x, y }, { x + 1.f, y + 1.f }, ColorU8_sRGB{ 120, 220, 140 } );
			}
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
			draw_text_5x7_( surface, { leftX, topY + 172.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "R: RESTART  M: AUDIO  ESC: QUIT" );
			draw_text_5x7_( surface, { leftX, topY + 196.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "TAB: TOGGLE MINIMAP" );
          draw_text_5x7_( surface, { leftX, topY + 220.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "P: PAUSE  F12: SAVE FRAME  F3: DEBUG" );

			draw_text_5x7_( surface, { rightX, topY }, 2, ColorU8_sRGB{ 170, 210, 255 }, "GAMEPLAY LOOP" );
			draw_text_5x7_( surface, { rightX, topY + 28.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "1. SURVIVE ASTEROIDS AND ENEMY FIRE" );
			draw_text_5x7_( surface, { rightX, topY + 52.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "2. CLEAR ENEMIES TO ADVANCE WAVES" );
			draw_text_5x7_( surface, { rightX, topY + 76.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "3. EVERY 4TH WAVE SPAWNS A BOSS" );
			draw_text_5x7_( surface, { rightX, topY + 100.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "4. SCORE UPGRADES YOUR FIREPOWER" );
			draw_text_5x7_( surface, { rightX, topY + 124.f }, 2, ColorU8_sRGB{ 250, 210, 90 }, "DIFFICULTY: 1 EASY  2 NORMAL  3 HARD" );
			draw_text_5x7_( surface, { rightX, topY + 148.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "COUNTDOWN START + SHIELD BAR ENABLED" );
			draw_text_5x7_( surface, { rightX, topY + 172.f }, 2, ColorU8_sRGB{ 230, 230, 230 }, "CLEARING WAVES RESTORES SHIELD" );
			if( state.highScore > 0 )
			{
				char hiScoreText[64] = {};
				std::snprintf( hiScoreText, sizeof(hiScoreText), "BEST SCORE: %d  BEST WAVE: %d", state.highScore, state.maxWaveReached );
				draw_text_5x7_( surface, { rightX, topY + 196.f }, 2, ColorU8_sRGB{ 255, 220, 100 }, hiScoreText );
			}

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
				{ fbwidth * 0.5f - 190.f, fbheight * 0.5f - 70.f },
				{ fbwidth * 0.5f + 190.f, fbheight * 0.5f + 70.f },
				ColorU8_sRGB{ 120, 20, 20 }
			);
			draw_rectangle_outline(
				surface,
				{ fbwidth * 0.5f - 190.f, fbheight * 0.5f - 70.f },
				{ fbwidth * 0.5f + 190.f, fbheight * 0.5f + 70.f },
				ColorU8_sRGB{ 255, 120, 120 }
			);
			draw_text_5x7_( surface, { fbwidth * 0.5f - 110.f, fbheight * 0.5f - 56.f }, 4, ColorU8_sRGB{ 255, 230, 230 }, "GAME OVER" );

			char finalScore[96] = {};
			std::snprintf( finalScore, sizeof(finalScore), "SCORE: %d  WAVE: %d  KILLS: %d", state.score, state.wave, state.totalKills );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 170.f, fbheight * 0.5f - 10.f }, 2, ColorU8_sRGB{ 255, 200, 160 }, finalScore );

			char timeText[64] = {};
			int minutes = int(state.totalPlayTime) / 60;
			int seconds = int(state.totalPlayTime) % 60;
			std::snprintf( timeText, sizeof(timeText), "TIME: %d:%02d  BOSSES: %d", minutes, seconds, state.bossesDefeated );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 148.f, fbheight * 0.5f + 16.f }, 2, ColorU8_sRGB{ 255, 200, 160 }, timeText );

			if( state.score >= state.highScore )
				draw_text_5x7_( surface, { fbwidth * 0.5f - 82.f, fbheight * 0.5f + 40.f }, 2, ColorU8_sRGB{ 255, 240, 100 }, "NEW HIGH SCORE" );

			draw_text_5x7_( surface, { fbwidth * 0.5f - 116.f, fbheight * 0.5f + 58.f }, 2, ColorU8_sRGB{ 255, 180, 180 }, "PRESS R TO RESTART" );
		}

		if( state.waveBannerTime > 0.f && !state.showStartScreen && !state.gameOver )
		{
			char waveText[64] = {};
			if( state.wave % 4 == 0 )
				std::snprintf( waveText, sizeof(waveText), "BOSS WAVE %d", state.wave );
			else
				std::snprintf( waveText, sizeof(waveText), "WAVE %d", state.wave );
			draw_rectangle_solid( surface, { fbwidth * 0.5f - 120.f, fbheight - 72.f }, { fbwidth * 0.5f + 120.f, fbheight - 40.f }, ColorU8_sRGB{ 24, 36, 72 } );
			draw_rectangle_outline( surface, { fbwidth * 0.5f - 120.f, fbheight - 72.f }, { fbwidth * 0.5f + 120.f, fbheight - 40.f }, ColorU8_sRGB{ 140, 190, 255 } );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 86.f, fbheight - 64.f }, 2, ColorU8_sRGB{ 240, 240, 180 }, waveText );
		}

		if( state.hitFlashTime > 0.f )
		{
			std::uint8_t const alphaTint = static_cast<std::uint8_t>( std::clamp( state.hitFlashTime * 1200.f, 24.f, 90.f ) );
			draw_rectangle_solid( surface, { 0.f, 0.f }, { float(fbwidth), float(fbheight) }, ColorU8_sRGB{ alphaTint, 28, 28 } );
		}

		if( state.paused && !state.showStartScreen && !state.gameOver )
		{
			draw_rectangle_solid( surface, { fbwidth * 0.5f - 130.f, fbheight * 0.5f - 24.f }, { fbwidth * 0.5f + 130.f, fbheight * 0.5f + 24.f }, ColorU8_sRGB{ 15, 24, 58 } );
			draw_rectangle_outline( surface, { fbwidth * 0.5f - 130.f, fbheight * 0.5f - 24.f }, { fbwidth * 0.5f + 130.f, fbheight * 0.5f + 24.f }, ColorU8_sRGB{ 130, 190, 255 } );
			draw_text_5x7_( surface, { fbwidth * 0.5f - 58.f, fbheight * 0.5f - 8.f }, 3, ColorU8_sRGB{ 220, 235, 255 }, "PAUSED" );
		}

		if( state.screenshotRequested )
		{
			auto const root = project_root_guess_();
			auto const stamp = static_cast<int>( state.totalPlayTime * 1000.f );
			auto const framePath = root / "artifacts" / ("frame-" + std::to_string( stamp ) + ".ppm");
			save_surface_ppm_( surface, framePath );
			state.screenshotRequested = false;
		}

		if( config.smokeTestSeconds > 0.f && !smokeCaptureDone && smokeElapsed >= config.smokeTestSeconds )
		{
			auto const root = project_root_guess_();
			auto const smokePath = root / "artifacts" / "smoketest-last-frame.ppm";
			save_surface_ppm_( surface, smokePath );
			smokeCaptureDone = true;
			glfwSetWindowShouldClose( window, GLFW_TRUE );
		}

		// Subtle screen edge vignette
		if( !state.showStartScreen )
		{
			float const vigW = 6.f;
			draw_rectangle_solid( surface, { 0.f, 0.f }, { vigW, float(fbheight) }, ColorU8_sRGB{ 4, 4, 8 } );
			draw_rectangle_solid( surface, { float(fbwidth) - vigW, 0.f }, { float(fbwidth), float(fbheight) }, ColorU8_sRGB{ 4, 4, 8 } );
			draw_rectangle_solid( surface, { 0.f, 0.f }, { float(fbwidth), vigW }, ColorU8_sRGB{ 4, 4, 8 } );
			draw_rectangle_solid( surface, { 0.f, float(fbheight) - vigW }, { float(fbwidth), float(fbheight) }, ColorU8_sRGB{ 4, 4, 8 } );
		}

		context.draw( surface );

		char title[320] = {};
		if( state.showStartScreen )
			std::snprintf( title, sizeof(title), "%s | Difficulty: %s (1/2/3) | Start: ENTER | HI: %d", kWindowTitle, state.difficulty == EDifficulty::easy ? "Easy" : (state.difficulty == EDifficulty::hard ? "Hard" : "Normal"), state.highScore );
		else if( state.countdownActive )
			std::snprintf( title, sizeof(title), "%s | %s | Launch in %.1fs", kWindowTitle, state.difficulty == EDifficulty::easy ? "Easy" : (state.difficulty == EDifficulty::hard ? "Hard" : "Normal"), state.countdownTime );
        else if( state.paused )
			std::snprintf( title, sizeof(title), "%s | PAUSED | Score: %d | Wave: %d | F12 Save Frame", kWindowTitle, state.score, state.wave );
        else
			std::snprintf( title, sizeof(title), "%s | Score: %d | Wave: %d | Kills: %d | Elite: %d | %.0f FPS%s", kWindowTitle, state.score, state.wave, state.totalKills, state.eliteKills, state.displayFps, state.gameOver ? " | GAME OVER (R)" : "" );
		glfwSetWindowTitle( window, title );

		glfwSwapBuffers( window );

	}

	// Cleanup.
	// For now, all objects are automatically cleaned up when they go out of
	// scope.
	save_profile_( state );
	
	return 0;
}
catch( std::exception const& eErr )
{
	std::fprintf( stderr, "Top-level Exception (%s):\n", typeid(eErr).name() );
	std::fprintf( stderr, "%s\n", eErr.what() );
	std::fprintf( stderr, "Bye.\n" );

#if defined(_WIN32)
	MessageBoxA( nullptr, eErr.what(), "StarWar startup error", MB_OK | MB_ICONERROR );
#endif

	return 1;
}

