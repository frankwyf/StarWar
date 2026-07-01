#pragma once

#include <filesystem>

#include "../draw2d/surface.hpp"
#include "state.hpp"

namespace starwar
{
    struct GameplayTuning
    {
        float enemyHpWaveScale = 0.08f;
        float eliteBaseChance = 0.05f;
        float eliteWaveChanceScale = 0.01f;
        float eliteMaxChance = 0.22f;
        float eliteDropChance = 0.30f;
        float playerBaseDamage = 1.f;
        float playerLv3Damage = 1.4f;
        float overdriveDuration = 5.f;
        int surgeEveryWaves = 5;
        float surgeDuration = 8.f;
    };

    std::filesystem::path project_root_guess();
    void load_profile( State& aState );
    GameplayTuning load_gameplay_tuning();
    void save_profile( State const& aState );
    void save_surface_ppm( Surface const& aSurface, std::filesystem::path const& aPath );
}
