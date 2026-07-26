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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(GameConstants::defaultWindowWidth, GameConstants::defaultWindowHeight,
               "Galaxy Impact");
    SetExitKey(KEY_NULL); // Escape is our own pause key, not the window-close key
    SetWindowMinSize(minWindowWidth, minWindowHeight);
    SetTargetFPS(targetFPS);

    InitAudioDevice();

    // Must run before InitGame() reads settings.txt/highscore.txt - on Web
    // this mounts+loads IndexedDB first; everywhere else it's a no-op.
    platformInitSaveData();

    Game game = InitGame();
    setupGuiTheme(game);

    if (const auto& opt = resolutionOptions.at(static_cast<size_t>(game.settings.resolutionIndex));
        opt.width != GameConstants::defaultWindowWidth ||
        opt.height != GameConstants::defaultWindowHeight)
    {
        SetWindowSize(opt.width, opt.height);
        syncScreenSize(game);
    }
    SetTargetFPS(fpsOptions.at(static_cast<size_t>(game.settings.fpsIndex)));
    applyBGMState(game);

    if (sandbox)
    {
        enterSandbox(game);
    }

    bool shouldExit = false;
    while (!WindowShouldClose() && !shouldExit)
    {
        const float deltaTime = GetFrameTime();

        shouldExit = UpdateGame(game, deltaTime);

        drawGame(game);
    }

    platformExitToLanding();

    UnloadMusicStream(game.bgm.drone);
    UnloadMusicStream(game.bgm.intensity);
    UnloadMusicStream(game.bgm.upgrade);
    UnloadRenderTexture(game.pixelTarget);
    UnloadRenderTexture(game.worldTarget);
    UnloadFont(game.font);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
