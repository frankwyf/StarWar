#include "persistence.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <string>

namespace starwar
{
    std::filesystem::path project_root_guess()
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

    namespace
    {
        std::filesystem::path profile_path_()
        {
            auto const root = project_root_guess();
            return root / "save" / "profile.cfg";
        }
    }

    void load_profile( State& aState )
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

    GameplayTuning load_gameplay_tuning()
    {
        GameplayTuning tuning;
        auto const root = project_root_guess();
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

        auto read_int = [&]( std::string const& key, int& value, std::string const& line )
        {
            auto const eq = line.find( '=' );
            if( eq == std::string::npos )
                return;
            auto const k = line.substr( 0, eq );
            if( k != key )
                return;
            value = std::atoi( line.substr( eq + 1 ).c_str() );
        };

        std::string line;
        while( std::getline( fin, line ) )
        {
            read_float( "enemy_hp_wave_scale", tuning.enemyHpWaveScale, line );
            read_float( "elite_base_chance", tuning.eliteBaseChance, line );
            read_float( "elite_wave_chance_scale", tuning.eliteWaveChanceScale, line );
            read_float( "elite_max_chance", tuning.eliteMaxChance, line );
            read_float( "elite_drop_chance", tuning.eliteDropChance, line );
            read_float( "player_base_damage", tuning.playerBaseDamage, line );
            read_float( "player_lv3_damage", tuning.playerLv3Damage, line );
            read_float( "overdrive_duration", tuning.overdriveDuration, line );
            read_int( "surge_every_waves", tuning.surgeEveryWaves, line );
            read_float( "surge_duration", tuning.surgeDuration, line );
        }

        tuning.eliteDropChance = std::clamp( tuning.eliteDropChance, 0.f, 1.f );
        tuning.overdriveDuration = std::clamp( tuning.overdriveDuration, 0.5f, 30.f );
        tuning.surgeEveryWaves = std::max( 2, tuning.surgeEveryWaves );
        tuning.surgeDuration = std::clamp( tuning.surgeDuration, 1.f, 30.f );

        return tuning;
    }

    void save_profile( State const& aState )
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

    void save_surface_ppm( Surface const& aSurface, std::filesystem::path const& aPath )
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
}
