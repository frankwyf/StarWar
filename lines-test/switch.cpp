#include <catch2/catch_amalgamated.hpp>

#include <algorithm>

#include "helpers.hpp"

#include "../draw2d/surface.hpp"
#include "../draw2d/draw.hpp"


TEST_CASE("Switch start and end points", "[connect]")
{
    Surface surface(640, 480); // Modified window size
    surface.clear();

    SECTION("horizontal")
    {
        // Original line
        draw_line_solid(surface,
            { 10.f, 170.f },
            { 110.f, 170.f },
            { 255, 255, 255 }
        );

        auto const countsOriginal = count_pixel_neighbours(surface);

        REQUIRE(2 == countsOriginal[1]);
        REQUIRE(countsOriginal[2] > 0);
        REQUIRE(0 == countsOriginal[0]);

        // Switched line
        surface.clear();
        draw_line_solid(surface,
            { 110.f, 170.f },
            { 10.f, 170.f },
            { 255, 255, 255 }
        );

        auto const countsSwitched = count_pixel_neighbours(surface);

        // Asserting the same tests after switching points
        REQUIRE(countsSwitched[1] == countsOriginal[1]);
        REQUIRE(countsSwitched[2] == countsOriginal[2]);
        REQUIRE(countsSwitched[0] == countsOriginal[0]);
        for (std::size_t i = 3; i < countsSwitched.size(); ++i)
            REQUIRE(countsSwitched[i] == countsOriginal[i]);
    }
    SECTION("vertical")
    {
        // Original line
        draw_line_solid(surface,
            { 64.f, 28.f },
            { 64.f, 100.f },
            { 255, 255, 255 }
        );

        auto const countsOriginal = count_pixel_neighbours(surface);
        REQUIRE(2 == countsOriginal[1]);
        REQUIRE(countsOriginal[2] > 0);
        REQUIRE(0 == countsOriginal[0]);

        // Switched line
        surface.clear();
        draw_line_solid(surface,
            { 64.f, 100.f },
            { 64.f, 28.f },
            { 255, 255, 255 }
        );

        auto const countsSwitched = count_pixel_neighbours(surface);

        // Asserting the same tests after switching points
        REQUIRE(countsSwitched[1] == countsOriginal[1]);
        REQUIRE(countsSwitched[2] == countsOriginal[2]);
        REQUIRE(countsSwitched[0] == countsOriginal[0]);
        for (std::size_t i = 3; i < countsSwitched.size(); ++i)
            REQUIRE(countsSwitched[i] == countsOriginal[i]);
    }

    SECTION("diagonal")
    {
        // Original line
        draw_line_solid(surface,
            { 10.f, 100.f },
            { 100.f, 10.f },
            { 255, 255, 255 }
        );

        auto const countsOriginal = count_pixel_neighbours(surface);
        REQUIRE(2 == countsOriginal[1]);
        REQUIRE(countsOriginal[2] > 0);
        REQUIRE(0 == countsOriginal[0]);

        // Switched line
        surface.clear();
        draw_line_solid(surface,
            { 100.f, 10.f },
            { 10.f, 100.f },
            { 255, 255, 255 }
        );

        auto const countsSwitched = count_pixel_neighbours(surface);

        // Asserting the same tests after switching points
        REQUIRE(countsSwitched[1] == countsOriginal[1]);
        REQUIRE(countsSwitched[2] == countsOriginal[2]);
        REQUIRE(countsSwitched[0] == countsOriginal[0]);
        for (std::size_t i = 3; i < countsSwitched.size(); ++i)
            REQUIRE(countsSwitched[i] == countsOriginal[i]);
    }
}