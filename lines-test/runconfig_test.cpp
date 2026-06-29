#include <catch2/catch_amalgamated.hpp>

#include "../support/runconfig.hpp"
#include "../support/error.hpp"

namespace
{
    RuntimeConfig parse_( std::initializer_list<char const*> args )
    {
        return parse_command_line( int(args.size()), args.begin() );
    }
}

TEST_CASE( "Runconfig defaults", "[runconfig]" )
{
    auto cfg = parse_( { "StarWar.exe" } );
    REQUIRE( cfg.initialWindowWidth == 1280u );
    REQUIRE( cfg.initialWindowHeight == 720u );
    REQUIRE( cfg.framebufferScaleShift == 0u );
    REQUIRE( cfg.selfTestAssets == false );
    REQUIRE( cfg.smokeTestSeconds == Catch::Approx(0.f) );
}

TEST_CASE( "Runconfig parses geometry/fbshift", "[runconfig]" )
{
    auto cfg = parse_( { "StarWar.exe", "--geometry=1920x1080", "--fbshift=1" } );
    REQUIRE( cfg.initialWindowWidth == 1920u );
    REQUIRE( cfg.initialWindowHeight == 1080u );
    REQUIRE( cfg.framebufferScaleShift == 1u );
}

TEST_CASE( "Runconfig parses automation flags", "[runconfig]" )
{
    auto cfg = parse_( { "StarWar.exe", "--selftest_assets", "--smoketest=0.75", "--seed=42" } );
    REQUIRE( cfg.selfTestAssets == true );
    REQUIRE( cfg.smokeTestSeconds == Catch::Approx(0.75f) );
   REQUIRE( cfg.fixedSeedEnabled == true );
    REQUIRE( cfg.fixedSeed == 42u );
}

TEST_CASE( "Runconfig rejects invalid values", "[runconfig]" )
{
    REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--geometry=abc" } ), Error );
    REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--fbshift=NaN" } ), Error );
    REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--smoketest=abc" } ), Error );
   REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--smoketest=-1" } ), Error );
    REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--smoketest=999" } ), Error );
   REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--seed=abc" } ), Error );
    REQUIRE_THROWS_AS( parse_( { "StarWar.exe", "--unknown" } ), Error );
}
