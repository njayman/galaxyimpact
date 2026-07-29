#include "draw.hpp"
#include "game.hpp"
#include "guitheme.hpp"
#include "platform.hpp"
#include "raylib.h"
#include "sandbox.hpp"
#include "settings.hpp"
#include "update.hpp"
#include <algorithm>
#include <cstring>

auto main(int argc, char* argv[]) -> int
{
    const bool sandbox = std::any_of(
        argv + 1, argv + argc, [](const char* arg)
        { return std::strcmp(arg, "-sandbox") == 0 || std::strcmp(arg, "--sandbox") == 0; });

    constexpr int minWindowWidth = 320;
    constexpr int minWindowHeight = 240;
    constexpr int targetFPS = 60;

#if defined(__EMSCRIPTEN__)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(GameConstants::defaultWindowWidth, GameConstants::defaultWindowHeight,
               "Galaxy Impact");
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT |
                   FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Galaxy Impact");
#endif
    SetExitKey(KEY_NULL);
    SetWindowMinSize(minWindowWidth, minWindowHeight);
    SetTargetFPS(targetFPS);

    InitAudioDevice();
    // Headroom margin: several BGM layers and SFX voices can play simultaneously and their
    // linear samples just sum in raylib's mixer, so trim the master bus to keep worst-case
    // overlap (rapid fire + explosion + boss music layers) from clipping.
    SetMasterVolume(0.85F);

    platformInitSaveData();

    Game game = InitGame();
    syncScreenSize(game);
    setupGuiTheme(game);

    SetTargetFPS(fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex)));
    applyBGMState(game);

    if (sandbox)
    {
        enterSandbox(game);
    }

    bool shouldExit = false;
    while (!WindowShouldClose() && !shouldExit)
    {
        if (IsWindowResized())
        {
            syncScreenSize(game);
        }

        const float deltaTime = GetFrameTime();

        shouldExit = UpdateGame(game, deltaTime);

        drawGame(game);
    }

    platformExitToLanding();

    UnloadMusicStream(game.resources.bgm.base);
    UnloadMusicStream(game.resources.bgm.intensity);
    UnloadMusicStream(game.resources.bgm.miniboss);
    UnloadMusicStream(game.resources.bgm.megaboss);
    UnloadMusicStream(game.resources.bgm.swarmBoss);
    UnloadSound(game.resources.sounds.shoot);
    UnloadSound(game.resources.sounds.hit);
    UnloadSound(game.resources.sounds.explosion);
    UnloadSound(game.resources.sounds.menuMove);
    UnloadSound(game.resources.sounds.menuConfirm);
    UnloadSound(game.resources.sounds.victory);
    UnloadSound(game.resources.sounds.defeat);
    UnloadSound(game.resources.sounds.critical);
    UnloadSound(game.resources.sounds.bossWindUp);
    UnloadSound(game.resources.sounds.beamFire);
    UnloadSound(game.resources.sounds.homingLaunch);
    UnloadSound(game.resources.sounds.spreadBurst);
    UnloadSound(game.resources.sounds.slamBoom);
    UnloadSound(game.resources.sounds.nerveCharge);
    UnloadSound(game.resources.sounds.nerveRelease);
    UnloadSound(game.resources.sounds.nerveFizzle);
    UnloadRenderTexture(game.resources.pixelTarget);
    UnloadRenderTexture(game.resources.worldTarget);
    UnloadFont(game.resources.font);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
