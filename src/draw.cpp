#include "draw_internal.hpp"

void drawText(const Font& font, const char* text, int32_t x, int32_t y, int32_t size, Color color)
{
    DrawTextEx(font, text, Vector2{.x = static_cast<float>(x), .y = static_cast<float>(y)},
               static_cast<float>(size), 1, color);
}

void drawText(const Game& game, const char* text, int32_t x, int32_t y, int32_t size, Color color)
{
    drawText(game.resources.font, text, x, y, size, color);
}

auto measureText(const Font& font, const char* text, int32_t size) -> int32_t
{
    return static_cast<int32_t>(MeasureTextEx(font, text, static_cast<float>(size), 1).x);
}

auto measureText(const Game& game, const char* text, int32_t size) -> int32_t
{
    return measureText(game.resources.font, text, size);
}

auto letterBoxRect(const Game& game) -> Rectangle
{
    const float scale = game.resources.renderScale;

    auto width = static_cast<float>(game.resources.screenWidth) * scale;
    auto height = static_cast<float>(game.resources.screenHeight) * scale;

    return Rectangle{.x = (static_cast<float>(game.resources.windowWidth) - width) / 2,
                     .y = (static_cast<float>(game.resources.windowHeight) - height) / 2,
                     .width = width,
                     .height = height};
}

namespace DrawDetail
{

void beginPixelZoom(const Game& game)
{
    // Stars are tiled centered on world-space origin (see drawBackgroundStars /
    // tiledWorldPos), so the camera needs the same origin-at-screen-center offset
    // beginWorldCamera uses - a zero offset put the whole starfield in the
    // top-left, empty everywhere else, since half the tiled range is negative.
    const float zoom = game.resources.renderScale;
    const Vector2 center{.x = static_cast<float>(game.resources.screenWidth) / 2,
                         .y = static_cast<float>(game.resources.screenHeight) / 2};
    BeginMode2D(Camera2D{.offset = Vector2Scale(center, zoom),
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = zoom});
}

auto transitionPush(const Game& game) -> float;

void beginWorldCamera(const Game& game)
{
    const float zoom = game.resources.renderScale * (1.0F + transitionCameraPush * transitionPush(game));
    const Vector2 center{.x = static_cast<float>(game.resources.screenWidth) / 2,
                         .y = static_cast<float>(game.resources.screenHeight) / 2};
    Vector2 offset = Vector2Scale(center, zoom);

    if (game.run.shakeTimer > 0 && game.run.shakeDuration > 0)
    {
        const float amplitude =
            game.run.shakeIntensity * (game.run.shakeTimer / game.run.shakeDuration);
        offset.x += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
        offset.y += static_cast<float>(GetRandomValue(-100, 100)) / 100 * amplitude * zoom;
    }

    BeginMode2D(Camera2D{
        .offset = offset, .target = game.run.player.position, .rotation = 0, .zoom = zoom});
}

auto tiledWorldPos(Vector2 relativeTo, Vector2 offset, float tileW, float tileH) -> Vector2
{
    float relX = std::fmod(offset.x - relativeTo.x, tileW);
    if (relX > tileW / 2)
    {
        relX -= tileW;
    }
    else if (relX < -tileW / 2)
    {
        relX += tileW;
    }

    float relY = std::fmod(offset.y - relativeTo.y, tileH);
    if (relY > tileH / 2)
    {
        relY -= tileH;
    }
    else if (relY < -tileH / 2)
    {
        relY += tileH;
    }

    return Vector2Add(relativeTo, Vector2{.x = relX, .y = relY});
}

auto biomeVoidColor(const Game& game) -> Color
{
    switch (currentBiome(game.run.waveNumber))
    {
    case Biome::ShatteredBelt:
        return Palette::Void;
    case Biome::Rustbloom:
        return Palette::RustbloomVoid;
    case Biome::SolarForge:
        return Palette::SolarForgeVoid;
    case Biome::Punctum:
        return Palette::PunctumVoid;
    }
    return Palette::Void;
}

auto biomeHazeColor(const Game& game) -> Color
{
    switch (currentBiome(game.run.waveNumber))
    {
    case Biome::ShatteredBelt:
        return Palette::Haze;
    case Biome::Rustbloom:
        return Palette::RustbloomHaze;
    case Biome::SolarForge:
        return Palette::SolarForgeHaze;
    case Biome::Punctum:
        return Palette::PunctumHaze;
    }
    return Palette::Haze;
}

void drawBackgroundStars(const Game& game, Vector2 relativeTo)
{
    const auto tileW = static_cast<float>(game.resources.screenWidth);
    const auto tileH = static_cast<float>(game.resources.screenHeight);

    for (const auto& p : game.run.bgParticles)
    {
        DrawCircleV(tiledWorldPos(relativeTo, p.position, tileW, tileH), p.radius, p.color);
    }

    const Color haze = biomeHazeColor(game);
    for (const auto& s : game.run.stars)
    {
        DrawCircleV(tiledWorldPos(relativeTo, s.position, tileW, tileH), s.radius, haze);
    }
}

void windowText(const Font& font, const Game& game, const char* text, int32_t centerX, int32_t y,
                int32_t size, Color color)
{
    const float scale = guiUiScale(game);
    const auto scaledSize = static_cast<int32_t>(static_cast<float>(size) * scale);
    const int32_t width = measureText(font, text, scaledSize);
    drawText(font, text, centerX - width / 2, static_cast<int32_t>(static_cast<float>(y) * scale),
             scaledSize, color);
}

void windowText(const Game& game, const char* text, int32_t centerX, int32_t y, int32_t size,
                Color color)
{
    windowText(game.resources.font, game, text, centerX, y, size, color);
}

// Pause screen: the Ascension passives held this run, left of the menu. Nothing until one is taken.
void drawAscensionList(const Game& game)
{
    const int32_t centerX = game.resources.windowWidth / 5;
    int32_t y = MenuLayout::pausedMenuY;
    for (size_t i = 0; i < ascensions.size(); i++)
    {
        const int stacks = game.run.ascensionStacks.at(i);
        if (stacks <= 0)
        {
            continue;
        }
        if (y == MenuLayout::pausedMenuY)
        {
            windowText(game, "ASCENSION", centerX, y, 22, Palette::Accent);
            y += 38;
        }
        windowText(game, std::format("{}  x{}", ascensions.at(i).name, stacks).c_str(), centerX, y, 18,
                   Palette::Charge);
        y += 28;
    }
}

void drawTitle(const Game& game)
{
    applyGuiScale(game);
    const TitleLayout layout = titleLayout();
    windowText(game, "GALAXY IMPACT", game.resources.windowWidth / 2, layout.titleY, 100,
              Palette::Accent);

    drawMenu(game, "", {"Start", "Skill Tree", "Achievements", "Bestiary", "Settings", "Exit"}, layout.menuY);
    drawHighScores(game, game.resources.windowWidth / 2, layout.highScoreY,
                   game.resources.highScores);
}

void drawShipSelect(const Game& game)
{
    applyGuiScale(game);
    const auto shipClass = static_cast<ShipClass>(game.menuIndex);
    const ShipDef& highlighted = ships.at(static_cast<size_t>(game.menuIndex));

    const ShipSelectLayout layout = shipSelectLayout(game);
    beginPop(layout.previewPos, popScale(game, 0) * easeOutBack(game.menuIndexAge / 0.22F));
    windowText(game, std::string(highlighted.name).c_str(), game.resources.windowWidth / 2,
              layout.nameY, 40, Palette::Accent);
    const Vector2 dir = aimAtMouse(game);
    const float previewAngle = std::atan2(dir.y, dir.x) + std::numbers::pi_v<float> / 2;
    if (shipClass != ShipClass::Bastion && shipClass != ShipClass::Interceptor &&
        shipClass != ShipClass::Ranger)
    {
        drawEngineFlame(layout.previewPos, layout.previewRadius, previewAngle);
    }
    drawShipHull(shipClass, layout.previewPos, layout.previewRadius, previewAngle,
                highlighted.color);
    endPop();

    const Vector2 mouse = GetMousePosition();
    const auto drawChevron = [&](Vector2 center, bool left, Rectangle tapZone)
    {
        const bool hovered = hoverActive(game) && CheckCollisionPointRec(mouse, tapZone);
        const Color color = hovered ? Palette::Accent : Palette::StructLight;
        const float size = layout.arrowHitRadius * 0.5F;
        const float sign = left ? -1.0F : 1.0F;
        const Vector2 tip{.x = center.x + sign * size, .y = center.y};
        const Vector2 top{.x = center.x - sign * size, .y = center.y - size};
        const Vector2 bottom{.x = center.x - sign * size, .y = center.y + size};
        const float thickness = 4.0F * guiUiScale(game);
        DrawLineEx(top, tip, thickness, color);
        DrawLineEx(tip, bottom, thickness, color);
    };
    drawChevron(layout.leftArrow, true, layout.leftTapZone);
    drawChevron(layout.rightArrow, false, layout.rightTapZone);

    windowText(game, std::string(highlighted.description).c_str(), game.resources.windowWidth / 2,
              layout.descY, 18, Palette::StructLight);
    if (!isShipUnlocked(game, shipClass))
    {
        windowText(game, "LOCKED - unlock it in the Skill Tree", game.resources.windowWidth / 2, layout.descY - 34, 20,
                   Palette::Accent);
    }

    const std::string stats = std::format(
        "HP {:.0f}   Armor {:.0f}   Dmg {:+.0f}%   Shields {}", highlighted.maxHealth,
        highlighted.armor, (highlighted.damageMult - 1.0F) * 100.0F, highlighted.maxShieldStacks);
    windowText(game, stats.c_str(), game.resources.windowWidth / 2, layout.statsY, 18,
              Palette::StructMid);

    const bool selectHovered = CheckCollisionPointRec(mouse, layout.selectRect);
    if (selectHovered)
    {
        GuiSetState(STATE_FOCUSED);
    }
    beginPop(Vector2{.x = layout.selectRect.x + layout.selectRect.width / 2,
                     .y = layout.selectRect.y + layout.selectRect.height / 2},
             popScale(game, 2));
    uiButton(layout.selectRect, "Select");
    endPop();
    GuiSetState(STATE_NORMAL);
}

void drawGameOver(const Game& game)
{
    applyGuiScale(game);
    beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2, .y = 90.0F * guiUiScale(game)},
             popScale(game, 0));
    windowText(game, "YOU WERE DEFEATED!", game.resources.windowWidth / 2, 70, 40, Palette::Accent);
    endPop();

    const std::string statsText =
        std::format("Score: {}   Level: {}   Wave: {}   Time: {:.0f}s", game.run.score,
                    game.run.level, game.run.waveNumber, game.run.runTime);
    windowText(game, statsText.c_str(), game.resources.windowWidth / 2, 120, 20,
               Palette::StructLight);

    drawHighScores(game, game.resources.windowWidth / 2, 160, game.resources.highScores);
    drawMenu(game, "", {"New Game", "Exit"}, MenuLayout::gameOverMenuY);
}

void drawEnding(const Game& game)
{
    const auto windowW = static_cast<float>(game.resources.windowWidth);
    const auto windowH = static_cast<float>(game.resources.windowHeight);
    const auto t = static_cast<float>(GetTime());
    const float parallax = game.run.endingTimer * endingDriftSpeed * 0.15F;

    for (int32_t i = 0; i < 60; i++)
    {
        const float seed = static_cast<float>(i);
        const float twinkle =
            0.35F + 0.35F * std::sin(t * (0.6F + hashNoise(seed) * 1.4F) + seed * 5.0F);
        const float x = std::fmod(hashNoise(seed * 3.1F) * windowW - parallax, windowW);
        DrawCircleV(Vector2{.x = x < 0 ? x + windowW : x, .y = hashNoise(seed * 7.7F) * windowH},
                    1.2F, Fade(WHITE, twinkle));
    }

    const float sunPulse = 1.0F + 0.06F * std::sin(t * 1.3F);
    const Vector2 sunPos{.x = windowW * 0.85F - parallax * 0.3F, .y = windowH * 0.2F};
    DrawCircleV(sunPos, 60 * sunPulse, Fade(Palette::Accent, 0.25F));
    DrawCircleV(sunPos, 42, Palette::Accent);

    const Vector2 earthPos{.x = windowW * 0.62F - parallax * 0.5F, .y = windowH * 0.42F};
    DrawCircleV(earthPos, 28, Palette::Shield);
    const float moonOrbit = t * 0.5F;
    const Vector2 moonPos = Vector2Add(
        earthPos, Vector2{.x = std::cos(moonOrbit) * 13.0F, .y = std::sin(moonOrbit) * 13.0F});
    DrawCircleV(moonPos, 9, Palette::Heal);

    const Vector2 driftPlanet1{.x = windowW * 0.3F - parallax * 0.8F,
                               .y = windowH * 0.72F + std::sin(t * 0.7F) * 6.0F};
    DrawCircleV(driftPlanet1, 9, Palette::StructMid);
    const Vector2 driftPlanet2{.x = windowW * 0.14F - parallax,
                               .y = windowH * 0.28F + std::sin(t * 0.9F + 2.0F) * 4.0F};
    DrawCircleV(driftPlanet2, 5, Palette::AccentDim);

    drawShip(game);

    applyGuiScale(game);
    constexpr float textFadeInDuration = 1.8F;
    const float textAlpha = std::clamp(game.run.endingTimer / textFadeInDuration, 0.0F, 1.0F);
    // windowText scales its y by guiUiScale(), so bottom-anchored positions must be
    // pre-divided by that scale or they land below the visible window on tall resolutions.
    const float scale = guiUiScale(game);
    const auto windowHeightF = static_cast<float>(game.resources.windowHeight);
    windowText(game, "Thank you for playing this game.", game.resources.windowWidth / 2,
               static_cast<int32_t>((windowHeightF - 140) / scale), 26, Fade(WHITE, textAlpha));
    windowText(game, "Never go to space alone.", game.resources.windowWidth / 2,
               static_cast<int32_t>((windowHeightF - 100) / scale), 26, Fade(WHITE, textAlpha));
    if (game.run.endingTimer >= endingSkipGraceDuration)
    {
        const float promptPulse = 0.6F + 0.4F * std::sin(t * 3.0F);
        windowText(game, "click to continue", game.resources.windowWidth / 2,
                   static_cast<int32_t>((windowHeightF - 50) / scale), 16,
                   Fade(Palette::StructLight, promptPulse));
    }
}

// --- Screen transitions --------------------------------------------------------
//
// Opening a screen grows a dark matte iris out of its anchor (the ship when it opens over
// gameplay, otherwise the pointer), while the world camera pushes in a little; the screen's
// items then pop in one after another with a slight overshoot. Returning to gameplay shrinks
// the iris back into the ship.
// -------------------------------------------------------------------------------

constexpr float irisOpenDuration = 0.32F;
constexpr float irisCloseDuration = 0.22F;

auto easeOutCubic(float t) -> float
{
    const float u = 1.0F - std::clamp(t, 0.0F, 1.0F);
    return 1.0F - u * u * u;
}

auto easeOutBack(float t) -> float
{
    constexpr float c1 = 1.70158F;
    constexpr float c3 = c1 + 1.0F;
    const float u = std::clamp(t, 0.0F, 1.0F) - 1.0F;
    return 1.0F + c3 * u * u * u + c1 * u * u;
}

// Screens drawn on top of the running world.
auto overGameplay(const Game& game, GameState state) -> bool
{
    switch (state)
    {
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::GAME_OVER:
    case GameState::SANDBOX_MENU:
        return true;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::WEAPON_GALLERY:
        return true;
    case GameState::SKILL_TREE:
        return game.settingsReturnState == GameState::PAUSED ||
               game.settingsReturnState == GameState::GAME_OVER ||
               game.settingsReturnState == GameState::SANDBOX_MENU;
    default:
        return false;
    }
}

auto shipWindowPos(const Game& game) -> Vector2
{
    const Rectangle box = letterBoxRect(game);
    return Vector2{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
}

void updateTransitions(Game& game)
{
    const float dt = GetFrameTime();
    if (game.state != game.drawnState)
    {
        const bool fromGameplay = game.drawnState == GameState::GAMEPLAY;
        if (game.state == GameState::GAMEPLAY && overGameplay(game, game.drawnState))
        {
            game.closeTimer = irisCloseDuration;
        }
        game.irisAnchor = fromGameplay || game.state == GameState::GAMEPLAY ? shipWindowPos(game) : GetMousePosition();
        game.drawnState = game.state;
        game.stateAge = 0;
    }
    else
    {
        game.stateAge += dt;
    }
    if (game.menuIndex != game.menuIndexDrawn)
    {
        game.menuIndexDrawn = game.menuIndex;
        game.menuIndexAge = 0;
    }
    else
    {
        game.menuIndexAge += dt;
    }
    game.closeTimer = std::max(0.0F, game.closeTimer - dt);
    game.qteAge = bossQteActive(game) ? game.qteAge + dt : 0;
}

auto irisMaxRadius(const Game& game, Vector2 anchor) -> float
{
    const auto w = static_cast<float>(game.resources.windowWidth);
    const auto h = static_cast<float>(game.resources.windowHeight);
    const float dx = std::max(anchor.x, w - anchor.x);
    const float dy = std::max(anchor.y, h - anchor.y);
    return std::sqrt(dx * dx + dy * dy) + 4.0F;
}

// How far the camera has pushed in on the ship (0..1).
auto transitionPush(const Game& game) -> float
{
    if (overGameplay(game, game.state))
    {
        return easeOutCubic(game.stateAge / irisOpenDuration);
    }
    return game.state == GameState::GAMEPLAY ? easeOutCubic(game.closeTimer / irisCloseDuration) : 0.0F;
}

void drawIrisRim(Vector2 anchor, float radius, float progress)
{
    if (progress < 1.0F)
    {
        DrawRing(anchor, radius - 2.0F, radius, 0, 360, 96, Fade(Palette::Haze, 0.5F * (1.0F - progress)));
    }
}

// The dimmed backdrop of a screen over gameplay: a dark disc growing out of the ship.
void drawOverlay(const Game& game)
{
    const float maxR = irisMaxRadius(game, game.irisAnchor);
    const float progress = easeOutCubic(game.stateAge / irisOpenDuration);
    if (progress >= 1.0F)
    {
        DrawRectangle(0, 0, game.resources.windowWidth, game.resources.windowHeight, Fade(Palette::Void, 0.75F));
        return;
    }
    DrawCircleV(game.irisAnchor, maxR * progress, Fade(Palette::Void, 0.75F));
    drawIrisRim(game.irisAnchor, maxR * progress, progress);
}

// A full screen (not over gameplay) is revealed through the iris: everything outside it stays dark.
void drawIrisReveal(const Game& game)
{
    const float progress = easeOutCubic(game.stateAge / irisOpenDuration);
    if (progress >= 1.0F || game.state == GameState::GAMEPLAY || game.state == GameState::ENDING ||
        overGameplay(game, game.state))
    {
        return;
    }
    const float maxR = irisMaxRadius(game, game.irisAnchor);
    const float r = maxR * progress;
    DrawRing(game.irisAnchor, r, maxR + 8.0F, 0, 360, 96, Palette::Void);
    drawIrisRim(game.irisAnchor, r, progress);
}

// Back to gameplay: the dark disc shrinks into the ship.
void drawIrisClose(const Game& game)
{
    if (game.state != GameState::GAMEPLAY || game.closeTimer <= 0)
    {
        return;
    }
    const float progress = easeOutCubic(game.closeTimer / irisCloseDuration);
    const float r = irisMaxRadius(game, game.irisAnchor) * progress;
    DrawCircleV(game.irisAnchor, r, Fade(Palette::Void, 0.75F * progress));
    drawIrisRim(game.irisAnchor, r, 1.0F - progress);
}

// Item `index` of the current screen pops in after the iris, staggered; 0 until its turn.
auto popScale(const Game& game, int32_t index) -> float
{
    const float t = (game.stateAge - irisOpenDuration * 0.4F - 0.05F * static_cast<float>(index)) / 0.24F;
    return t <= 0 ? 0.0F : easeOutBack(t);
}

void beginPop(Vector2 center, float scale)
{
    rlDrawRenderBatchActive(); // flush what came before, so only this item is scaled
    rlPushMatrix();
    rlTranslatef(center.x, center.y, 0);
    rlScalef(scale, scale, 1);
    rlTranslatef(-center.x, -center.y, 0);
}

void endPop()
{
    rlDrawRenderBatchActive();
    rlPopMatrix();
}

} // namespace DrawDetail

using namespace DrawDetail;

auto drawGame(Game& game) -> void
{
    updateTransitions(game);
    preparePunctumInfallSprites(game); // renders into its own textures, so before the frame's target
    BeginTextureMode(game.resources.worldTarget);
    ClearBackground(biomeVoidColor(game));

    switch (game.state)
    {
    case GameState::TITLE:
    case GameState::ENDING:
        beginPixelZoom(game);
        drawBackgroundStars(game, Vector2{});
        EndMode2D();
        break;
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::GAME_OVER:
    case GameState::SANDBOX_MENU:
        beginWorldCamera(game);
        drawGameplayWorld(game);
        EndMode2D();
        break;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::SKILL_TREE:
    case GameState::WEAPON_GALLERY:
        if (overGameplay(game, game.state))
        {
            beginWorldCamera(game);
            drawGameplayWorld(game);
        }
        else
        {
            beginPixelZoom(game);
            drawBackgroundStars(game, Vector2{});
        }
        EndMode2D();
        break;
    }

    EndTextureMode();

    BeginTextureMode(game.resources.pixelTarget);
    ClearBackground(Palette::Void);

    const auto worldSrc =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.worldTarget.get().texture.width),
                  .height = -static_cast<float>(game.resources.worldTarget.get().texture.height)};
    const auto worldDst =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.worldTarget.get().texture.width),
                  .height = static_cast<float>(game.resources.worldTarget.get().texture.height)};
    DrawTexturePro(game.resources.worldTarget.get().texture, worldSrc, worldDst, Vector2{}, 0,
                   WHITE);

    EndTextureMode();

    BeginDrawing();
    ClearBackground(Palette::Void);

    for (size_t i = 0; i < game.run.borderStars.size(); ++i)
    {
        auto gameBorderStar = game.run.borderStars[i];
        if (gameBorderStar.position.x <= static_cast<float>(game.resources.windowWidth) &&
            gameBorderStar.position.y <= static_cast<float>(game.resources.windowHeight))
        {
            DrawCircleV(gameBorderStar.position, gameBorderStar.radius, Palette::Haze);
        }
    }

    const auto srcRec =
        Rectangle{.x = 0,
                  .y = 0,
                  .width = static_cast<float>(game.resources.pixelTarget.get().texture.width),
                  .height = -static_cast<float>(game.resources.pixelTarget.get().texture.height)};
    const Rectangle letterbox = letterBoxRect(game);
    DrawTexturePro(game.resources.pixelTarget.get().texture, srcRec, letterbox, Vector2{}, 0,
                   WHITE);

    const float uiScale = letterbox.width / static_cast<float>(game.resources.screenWidth);
    BeginMode2D(Camera2D{.offset = Vector2{.x = letterbox.x, .y = letterbox.y},
                         .target = Vector2{},
                         .rotation = 0,
                         .zoom = uiScale * hudScale(game)});

    switch (game.state)
    {
    case GameState::GAMEPLAY:
    case GameState::PAUSED:
    case GameState::LEVEL_UP:
    case GameState::SANDBOX_MENU:
        drawHUD(game);
        break;
    case GameState::SHIP_SELECT:
    case GameState::SETTINGS:
    case GameState::ACHIEVEMENTS:
    case GameState::BESTIARY:
    case GameState::SKILL_TREE:
    case GameState::WEAPON_GALLERY:
        if (game.settingsReturnState == GameState::PAUSED)
        {
            drawHUD(game);
        }
        break;
    case GameState::TITLE:
    case GameState::GAME_OVER:
    case GameState::ENDING:
        break;
    }

    EndMode2D();

    switch (game.state)
    {
    case GameState::TITLE:
        drawTitle(game);
        break;
    case GameState::SHIP_SELECT:
        if (game.settingsReturnState != GameState::TITLE)
        {
            drawOverlay(game);
        }
        drawShipSelect(game);
        break;
    case GameState::GAMEPLAY:
        if (game.sandbox && !bossQteActive(game))
        {
            drawSandboxToggleIndicator(game);
        }
        drawTouchControls(game);
        break;
    case GameState::SANDBOX_MENU:
        drawOverlay(game);
        drawSandboxMenu(game);
        break;
    case GameState::PAUSED:
        drawOverlay(game);
        drawMenu(game, "PAUSED",
                 {"Resume", "Achievements", "Bestiary", "Settings", "New Game", "Exit"},
                 MenuLayout::pausedMenuY);
        drawAscensionList(game);
        break;
    case GameState::LEVEL_UP:
        drawOverlay(game);
        drawLevelUp(game);
        break;
    case GameState::GAME_OVER:
        drawOverlay(game);
        drawGameOver(game);
        break;
    case GameState::SETTINGS:
        drawOverlay(game);
        drawSettings(game);
        break;
    case GameState::ACHIEVEMENTS:
        drawOverlay(game);
        drawAchievements(game);
        break;
    case GameState::BESTIARY:
        drawOverlay(game);
        drawBestiary(game);
        break;
    case GameState::SKILL_TREE:
        drawOverlay(game);
        drawSkillTree(game);
        break;
    case GameState::WEAPON_GALLERY:
        drawOverlay(game);
        drawWeaponGallery(game);
        break;
    case GameState::ENDING:
        drawEnding(game);
        break;
    }
    drawIrisReveal(game);
    drawIrisClose(game);

    EndDrawing();
}
