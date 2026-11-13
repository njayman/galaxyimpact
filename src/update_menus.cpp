#include "update_internal.hpp"

auto updateTitle(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 6, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, titleLayout().menuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::SHIP_SELECT;
            break;
        case 1:
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::SKILL_TREE;
            break;
        case 2:
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::ACHIEVEMENTS;
            break;
        case 3:
            game.settingsReturnState = GameState::TITLE;
            game.state = GameState::BESTIARY;
            break;
        case 4:
            game.settingsReturnState = GameState::TITLE;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 5:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateAchievements(Game& game) -> bool
{
    applyScrollInput(game.achievementsScroll);

    // Esc/Enter or the Back button close it; a tap-and-hold previews a
    // weapon's unlock details instead (see hoverActive). A press-edge check
    // (not a release-edge one like quickTapReleased) is required here: the
    // click that opened this screen still has its release pending on this
    // screen's first frame, and a release-triggered close would misfire on
    // that leftover release.
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
         CheckCollisionPointRec(GetMousePosition(), backButtonRect(game))))
    {
        game.state = game.settingsReturnState;
        game.achievementsScroll.offset = 0;
    }
    return false;
}

// Skill tree: hover or arrow keys pick a node, click or Enter buys it.
auto updateSkillTree(Game& game) -> bool
{
    if (game.skillTreePopTimer > 0)
    {
        game.skillTreePopTimer -= GetFrameTime();
    }
    if (IsKeyPressed(KEY_ESCAPE) || (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                                     CheckCollisionPointRec(GetMousePosition(), backButtonRect(game))))
    {
        game.state = game.settingsReturnState;
        return false;
    }

    const int32_t before = game.skillTreeSelected;
    bool pointerOnSelected = false;
    const float radius = skillTreeNodeRadius(game);
    const Vector2 mouse = GetMousePosition();
    for (int32_t i = 0; i < skillNodeCount; i++)
    {
        if (Vector2Distance(mouse, skillTreeNodePos(game, i)) < radius * 1.2F)
        {
            // Hover only when the mouse moves, so a resting cursor doesn't fight the arrow keys.
            if ((hoverActive(game) && Vector2Length(GetMouseDelta()) > 0) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                game.skillTreeSelected = i;
            }
            pointerOnSelected = pointerOnSelected || i == game.skillTreeSelected;
        }
    }

    // Arrow keys: the nearest node roughly in that direction.
    const auto axis = [](int32_t plus, int32_t plusAlt, int32_t minus, int32_t minusAlt)
    {
        return (IsKeyPressed(plus) || IsKeyPressed(plusAlt) ? 1.0F : 0.0F) -
               (IsKeyPressed(minus) || IsKeyPressed(minusAlt) ? 1.0F : 0.0F);
    };
    const Vector2 dir{.x = axis(KEY_RIGHT, KEY_D, KEY_LEFT, KEY_A), .y = axis(KEY_DOWN, KEY_S, KEY_UP, KEY_W)};
    if (dir.x != 0 || dir.y != 0)
    {
        const Vector2 from = skillTreeNodePos(game, game.skillTreeSelected);
        float best = 1e9F;
        for (int32_t i = 0; i < skillNodeCount; i++)
        {
            const Vector2 to = Vector2Subtract(skillTreeNodePos(game, i), from);
            const float along = Vector2DotProduct(to, Vector2Normalize(dir));
            const float dist = Vector2Length(to);
            if (i == before || along < dist * 0.5F)
            {
                continue;
            }
            const float score = dist + (dist - along) * 2.0F; // prefer straight ahead
            if (score < best)
            {
                best = score;
                game.skillTreeSelected = i;
            }
        }
    }
    if (game.skillTreeSelected != before)
    {
        playSFX(game, game.resources.sounds.menuMove);
    }

    // A click only selects (to read it); holding the mouse on the node, or Enter/Space, unlocks.
    const int32_t sel = game.skillTreeSelected;
    const bool canBuy = skillNodeAvailable(game, sel) && (game.sandbox || skillPoints(game) > 0);
    const bool holding = (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && pointerOnSelected) || IsKeyDown(KEY_ENTER) ||
                         IsKeyDown(KEY_SPACE);
    const bool pressed = (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && pointerOnSelected && sel == before) ||
                         IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    if (pressed && !canBuy && !skillNodeOwned(game, sel))
    {
        playSFX(game, game.resources.sounds.nerveFizzle);
    }
    if (holding && canBuy && sel == before)
    {
        game.skillTreeHold += GetFrameTime();
        if (game.skillTreeHold >= skillTreeHoldDuration && buySkillNode(game, sel))
        {
            playSFX(game, game.resources.sounds.menuConfirm);
            game.skillTreePopNode = sel;
            game.skillTreePopTimer = 0.45F;
            game.skillTreeHold = 0;
        }
    }
    else
    {
        game.skillTreeHold = 0;
    }
    return false;
}

auto updateBestiary(Game& game) -> bool
{
    applyScrollInput(game.bestiaryScroll);

    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER) ||
        (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
         CheckCollisionPointRec(GetMousePosition(), backButtonRect(game))))
    {
        game.state = game.settingsReturnState;
        game.bestiaryScroll.offset = 0;
    }
    return false;
}

auto updateShipSelect(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.menuIndex = 0;
        game.state = game.settingsReturnState;
        return false;
    }

    const auto count = static_cast<int32_t>(ShipClass::Count);
    int32_t newIndex = game.menuIndex;
    bool confirmed = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT))
    {
        newIndex = (newIndex - 1 + count) % count;
    }
    if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT))
    {
        newIndex = (newIndex + 1) % count;
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        const ShipSelectLayout layout = shipSelectLayout(game);
        const Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, layout.leftTapZone))
        {
            newIndex = (newIndex - 1 + count) % count;
        }
        else if (CheckCollisionPointRec(mouse, layout.rightTapZone))
        {
            newIndex = (newIndex + 1) % count;
        }
        else if (CheckCollisionPointRec(mouse, layout.selectRect))
        {
            confirmed = true;
        }
    }

    playMenuSounds(game, newIndex, confirmed);
    game.menuIndex = newIndex;

    if (confirmed && !isShipUnlocked(game, static_cast<ShipClass>(newIndex)))
    {
        confirmed = false; // locked: unlocked in the skill tree
        playSFX(game, game.resources.sounds.nerveFizzle);
    }
    if (confirmed)
    {
        game.resources.settings.shipIndex = newIndex;
        saveSettings(game.resources.settings);
        resetRun(game);
    }

    return false;
}

auto updatePaused(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.state = GameState::GAMEPLAY;
        return false;
    }

    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 6, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::pausedMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.state = GameState::GAMEPLAY;
            break;
        case 1:
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::ACHIEVEMENTS;
            break;
        case 2:
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::BESTIARY;
            break;
        case 3:
            game.settingsReturnState = GameState::PAUSED;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 4:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::PAUSED;
            game.state = GameState::SHIP_SELECT;
            break;
        case 5:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateEnding(Game& game, float deltaTime) -> bool
{
    game.run.player.position.x -= endingDriftSpeed * deltaTime;
    game.run.player.dashing = false;
    game.run.player.shieldActive = false;
    game.run.player.nerveCharging = false;

    game.run.endingTimer += deltaTime;

    const bool skipRequested =
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    // ponytail: the dash that kills the final boss also fires on MOUSE_BUTTON_LEFT, so a short
    // grace window keeps that same click from instantly skipping the scene it just triggered.
    if ((skipRequested && game.run.endingTimer >= endingSkipGraceDuration) ||
        game.run.endingTimer >= endingDuration)
    {
        game.state = GameState::TITLE;
        game.menuIndex = 0;
    }
    return false;
}

auto updateGameOver(Game& game) -> bool
{

    if (!game.run.scoreRecorded && !game.sandbox)
    {
        game.resources.highScores = highscore::record(*game.resources.highScoreRepo,
                                                      game.resources.highScores, game.run.score);
        game.run.scoreRecorded = true;
    }

    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 2, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::gameOverMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            game.menuIndex = game.resources.settings.shipIndex;
            game.settingsReturnState = GameState::GAME_OVER;
            game.state = GameState::SHIP_SELECT;
            break;
        case 1:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateLevelUp(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, static_cast<int32_t>(game.run.pendingChoices.size()),
        MenuLayout::levelUpWidth, MenuLayout::levelUpHeight, MenuLayout::levelUpGap,
        MenuLayout::levelUpMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        const auto& choice = game.run.pendingChoices.at(static_cast<size_t>(index));
        switch (choice.type)
        {
        case ChoiceType::Evolve:
            applyEvolution(game, choice.weapon.value());
            break;
        case ChoiceType::Pickup:
            applyPickupEffect(game, choice.pickupType, choice.element, choice.mechanism);
            break;
        case ChoiceType::Ascension:
            applyAscension(game, choice.ascension);
            break;
        case ChoiceType::Skill:
        default:
            applySkill(game, choice.skill);
            break;
        }
        game.state = GameState::GAMEPLAY;
    }

    return false;
}

void playMenuSounds(Game& game, int32_t newIndex, bool confirmed)
{
    if (confirmed)
    {
        playSFX(game, game.resources.sounds.menuConfirm);
    }
    else if (newIndex != game.menuIndex)
    {
        playSFX(game, game.resources.sounds.menuMove);
    }
}

void playSFX(Game& game, Sound sound)
{
    if (!game.resources.settings.soundOn)
    {
        return;
    }

    // Mix guard. A raylib Sound is one voice (replaying restarts it), so this throttles each sound
    // and caps how many different ones sound at once: busy scenes cannot stack into clipping or bury the BGM.
    // ponytail: no priorities, a late sound is simply dropped; add a priority flag if a boss tell ever goes missing.
    struct Voice
    {
        Sound sound{};
        double lastPlayTime = -1000.0;
    };
    static std::array<Voice, 48> voices{};

    const double now = GetTime();
    Voice* slot = nullptr;
    int playing = 0;
    for (auto& voice : voices)
    {
        if (voice.sound.stream.buffer == nullptr)
        {
            slot = slot != nullptr ? slot : &voice;
            break;
        }
        if (voice.sound.stream.buffer == sound.stream.buffer)
        {
            slot = &voice;
        }
        else if (IsSoundPlaying(voice.sound))
        {
            playing++;
        }
    }
    const bool known = slot != nullptr && slot->sound.stream.buffer == sound.stream.buffer;
    if ((known && now - slot->lastPlayTime < sfxThrottleInterval) || playing >= sfxMaxVoices)
    {
        return;
    }
    if (slot != nullptr)
    {
        slot->sound = sound;
        slot->lastPlayTime = now;
    }
    PlaySound(sound);
}

// Swap Sticks only matters with touch controls, so its row (setting 6) shows only then.
auto settingsRows(const Game& game) -> int32_t { return touchInputActive(game) ? settingsRowCount : settingsRowCount - 1; }

auto settingsRowSetting(const Game& game, int32_t row) -> int32_t
{
    return !touchInputActive(game) && row >= 6 ? row + 1 : row;
}

auto updateSettings(Game& game) -> bool
{
    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.state = game.settingsReturnState;
        game.menuIndex = 0;
        return false;
    }
    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP))
    {
        game.menuIndex--;
    }
    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN))
    {
        game.menuIndex++;
    }
    if (game.menuIndex < 0)
    {
        game.menuIndex = settingsRows(game) - 1;
    }
    if (game.menuIndex >= settingsRows(game))
    {
        game.menuIndex = 0;
    }

    const bool left = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    const bool right = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    const Vector2 mouseDelta = GetMouseDelta();
    const bool mouseMoved =
        std::abs(mouseDelta.x) > FLT_EPSILON || std::abs(mouseDelta.y) > FLT_EPSILON;
    if (mouseMoved)
    {
        if (const auto row = hoveredColumnRow(settingsRows(game), MenuLayout::settingsWidth,
                                              MenuLayout::settingsHeight, MenuLayout::settingsGap,
                                              MenuLayout::settingsMenuY, game);
            row.has_value())
        {
            game.menuIndex = *row;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (const auto row = hoveredColumnRow(settingsRows(game), MenuLayout::settingsWidth,
                                              MenuLayout::settingsHeight, MenuLayout::settingsGap,
                                              MenuLayout::settingsMenuY, game);
            row.has_value())
        {
            game.menuIndex = *row;
            confirm = true;
        }
    }

    if (left || right || confirm)
    {
        playSFX(game, game.resources.sounds.menuMove);
    }

    const int32_t setting = settingsRowSetting(game, game.menuIndex);
    switch (setting)
    {
    case 0:
        if (left)
        {
            cycleResolution(game, -1);
        }
        if (right || confirm)
        {
            cycleResolution(game, 1);
        }
        break;
    case 1:
        if (left || right || confirm)
        {
            game.resources.settings.bgmOn = !game.resources.settings.bgmOn;
            applyBGMState(game);
        }
        break;
    case 2:
        if (left || right || confirm)
        {
            game.resources.settings.soundOn = !game.resources.settings.soundOn;
        }
        break;
    case 3:
        if (left)
        {
            cycleFPS(game, -1);
        }
        if (right || confirm)
        {
            cycleFPS(game, 1);
        }
        break;
    case 4:
        if (left)
        {
            cycleHudScale(game, -1);
        }
        if (right || confirm)
        {
            cycleHudScale(game, 1);
        }
        break;
    case 5:
        if (left || right || confirm)
        {
            toggleFullscreen(game);
        }
        break;
    case 6:
        if (left || right || confirm)
        {
            game.resources.settings.swapTouchSticks = !game.resources.settings.swapTouchSticks;
        }
        break;
    case 7:
        if (confirm)
        {
            game.state = game.settingsReturnState;
            game.menuIndex = 0;
        }
        break;
    default:
        break;
    }

    if ((left || right || confirm) && (setting <= 4 || setting == 6))
    {
        saveSettings(game.resources.settings);
    }

    return false;
}

void cycleResolution(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(resolutionOptions.size());
    game.resources.settings.resolutionIndex =
        (game.resources.settings.resolutionIndex + dir + n) % n;
    const auto& opt =
        resolutionOptions.at(static_cast<size_t>(game.resources.settings.resolutionIndex));
    SetWindowSize(opt.width, opt.height);
    syncScreenSize(game);
}

void cycleFPS(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(fpsOptions.size());
    game.resources.settings.fpsIndex = (game.resources.settings.fpsIndex + dir + n) % n;
    SetTargetFPS(fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex)));
}

void cycleHudScale(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(hudScaleOptions.size());
    game.resources.settings.hudScaleIndex = (game.resources.settings.hudScaleIndex + dir + n) % n;
}

void applyBGMState(Game& game)
{
    if (game.resources.settings.bgmOn)
    {
        ResumeMusicStream(game.resources.bgm.base);
        ResumeMusicStream(game.resources.bgm.intensity);
        ResumeMusicStream(game.resources.bgm.miniboss);
        for (const Music& track : game.resources.bgm.megaboss)
        {
            ResumeMusicStream(track);
        }
        ResumeMusicStream(game.resources.bgm.swarmBoss);
    }
    else
    {
        PauseMusicStream(game.resources.bgm.base);
        PauseMusicStream(game.resources.bgm.intensity);
        PauseMusicStream(game.resources.bgm.miniboss);
        for (const Music& track : game.resources.bgm.megaboss)
        {
            PauseMusicStream(track);
        }
        PauseMusicStream(game.resources.bgm.swarmBoss);
    }
}

void updateBgmLayers(Game& game, float deltaTime)
{
    constexpr float intensityEnemyThreshold = 25.0F;
    constexpr float volumeSmoothing = 1.5F;
    constexpr float calmCeiling = 0.35F;
    constexpr float swarmOverrideThreshold = 0.7F;

    constexpr float intensityFloor = 0.3F;

    float intensityTarget =
        std::clamp(static_cast<float>(game.run.enemies.size()) / intensityEnemyThreshold,
                   intensityFloor, 1.0F);

    if (game.resources.bgm.calmTimer > 0)
    {
        if (intensityTarget >= swarmOverrideThreshold)
        {
            game.resources.bgm.calmTimer = 0;
        }
        else
        {
            intensityTarget = std::min(intensityTarget, calmCeiling);
            game.resources.bgm.calmTimer -= deltaTime;
        }
    }

    constexpr float minibossFullVolume = 0.6F;
    constexpr float megabossFullVolume = 0.8F;
    constexpr float swarmBossFullVolume = 1.0F;

    bool minibossActive = false;
    bool megabossActive = false;
    bool swarmBossActive = false;
    for (const auto& boss : game.run.bosses)
    {
        if (boss.isSwarm)
        {
            swarmBossActive = true;
        }
        else if (boss.isMega)
        {
            megabossActive = true;
        }
        else
        {
            minibossActive = true;
        }
    }
    const float swarmBossTarget = swarmBossActive ? swarmBossFullVolume : 0.0F;
    const float megabossTarget = (megabossActive && !swarmBossActive) ? megabossFullVolume : 0.0F;
    const float minibossTarget =
        (minibossActive && !swarmBossActive && !megabossActive) ? minibossFullVolume : 0.0F;

    const float rate = 1 - std::exp(-volumeSmoothing * deltaTime);
    game.resources.bgm.intensityVolume +=
        (intensityTarget - game.resources.bgm.intensityVolume) * rate;
    game.resources.bgm.minibossVolume +=
        (minibossTarget - game.resources.bgm.minibossVolume) * rate;
    game.resources.bgm.megabossVolume +=
        (megabossTarget - game.resources.bgm.megabossVolume) * rate;
    game.resources.bgm.swarmBossVolume +=
        (swarmBossTarget - game.resources.bgm.swarmBossVolume) * rate;

    float duckFactor = 1.0F;
    if (game.resources.bgm.duckTimer > 0)
    {
        game.resources.bgm.duckTimer -= deltaTime;
        const float t = std::clamp(game.resources.bgm.duckTimer / bgmDuckDuration, 0.0F, 1.0F);
        duckFactor = 1.0F - bgmDuckStrength * t;
    }

    SetMusicVolume(game.resources.bgm.base, SoundConstants::baseVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.intensity, game.resources.bgm.intensityVolume * duckFactor);
    SetMusicVolume(game.resources.bgm.miniboss, game.resources.bgm.minibossVolume * duckFactor);
    // Only the current biome's boss track is audible; all four keep running so they stay in step.
    const auto biome = static_cast<size_t>(currentBiome(game.run.waveNumber));
    for (size_t i = 0; i < game.resources.bgm.megaboss.size(); i++)
    {
        SetMusicVolume(game.resources.bgm.megaboss.at(i),
                       i == biome ? game.resources.bgm.megabossVolume * duckFactor : 0.0F);
    }
    SetMusicVolume(game.resources.bgm.swarmBoss, game.resources.bgm.swarmBossVolume * duckFactor);
}
