#include "update.hpp"

#include "democonfig.hpp"
#include "draw.hpp"
#include "entities/item.hpp"
#include "highscore.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "raylib.h"
#include "raymath.h"
#include "sandbox.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <vector>

// frameScale converts the many speed/pull constants below - all originally
// tuned as an implicit per-frame displacement assuming 60fps - into
// per-second terms without rederiving every one by hand: multiplying a
// per-frame delta by (deltaTime * frameScale) reproduces the original
// 60fps-tuned distance exactly at 60fps, and scales correctly at the 120/240
// FPS caps instead of moving proportionally faster.
constexpr float frameScale = UpdateConstants::frameScale;

constexpr float projectileSpeed = 10;
constexpr float projectileSize = 7;
constexpr int32_t bossRamDamage = 50;
constexpr float blackHolePull = 2;
constexpr float blackHoleSlow = 2.5F;
constexpr float blackHoleAsteroidPull = 1.5F;
constexpr float blackHoleChaseSpeed = 0.3F;
// homingProjSpeed/spreadProjSpeed: comparable to the player's own baseline
// move speed (game.player.speed = 5, up to ~6 with max Nerve, more with
// Thrusters) so these don't trail hopelessly behind a player running in the
// same direction - previously well under it (2.5/4), letting any straight
// retreat trivially outrun them regardless of aim.
constexpr float homingProjSpeed = 6.5F;
constexpr float spreadProjSpeed = 6;
constexpr int32_t baseBulletDamage = 5;
constexpr float pickupMagnetSpeed = 7;
constexpr float dashSpeed = 22;
constexpr float dashDuration = 0.18F;
constexpr int32_t dashDamage = 15;
constexpr float dashKillChargeRefund = 0.6F;
constexpr float shieldKillChargeRefund = 0.6F;
constexpr float bossBodyLingerLimit = 1.0F; // grace window before lingering inside a boss kills you
constexpr float emergencyActionNerveThreshold = 0.7F; // fraction of nerveMax required to trigger
constexpr float damageMeterHoldDuration = 1.5F;

// Charger enemy (EnemyPattern::Charge): telegraph window before the dash
// actually moves, and the dash's own duration once it does.
constexpr float chargeTelegraphDuration = 0.5F;
constexpr float enemyChargeDashDuration = UpdateConstants::enemyChargeDashDuration;
// Pickup expiry: XP is common/low-stakes and expires quickly; Shield/LifeOrb
// are rarer and worth chasing a bit longer. Both blink in their last
// pickupExpiryWarning seconds as a despawn warning.
constexpr float xpPickupLifetime = 10.0F;
constexpr float bonusPickupLifetime = 18.0F;
constexpr float shieldBaseDuration = 1.2F;
constexpr float entityDespawnRadius = 1400;
constexpr float asteroidDespawnRadius = 900;
constexpr float turretFireRange = 700;
constexpr int32_t crossfireProjectileDamage = 15;
constexpr int32_t shieldDropChance = 15;  // 1.5%
constexpr int32_t lifeOrbDropChance = 40; // 4%, i.e. rolls 15..54
constexpr int32_t settingsRowCount =
    7; // Resolution, Difficulty, BGM, Sound, FPS, Display Mode, Back
constexpr float nerveKillGain = 6;
constexpr float nerveDecayPerSec = 4;
constexpr float nerveDamageBonusMax = 0.5F;
constexpr float nerveSpeedBonusMax = 0.2F;
constexpr float postCapDamageBonusPerLevel = 0.05F;
constexpr float mineLifetime = 5.0F;
constexpr float mineHomeSpeed = 3.5F;
constexpr float mineSeekRadius = 320;
constexpr float bossEngageDistance = 380;
constexpr float bossKillCalmDuration = 6.0F;

// Boss cadence: a miniboss every 5 waves, a mega boss every 10 (which
// supersedes the miniboss on waves divisible by both). Every 50th boss
// spawn (mini or mega) is a swarm of 3 instead of 1.
constexpr int miniBossWaveInterval = 5;
constexpr int megaBossWaveInterval = 10;
constexpr int bossSwarmInterval = 50;
constexpr float miniBossHealthMult = 0.5F;
constexpr float miniBossSizeMult = 0.7F;
constexpr float megaBossHealthMult = 1.5F;
constexpr float megaBossSizeMult = 1.3F;

// Boss kill rewards: miniboss XP is enough to guarantee (and slightly
// overshoot, for a felt "boost") one level-up; megaboss overshoots enough to
// usually chain a second level-up next frame once xpToNext is recalculated.
// A swarm kill (every bossSwarmInterval-th spawn) additionally drops a
// guaranteed Shield+LifeOrb jackpot per boss on top of its mega reward.
constexpr float miniBossXpMult = 1.5F;
constexpr float megaBossXpMult = 2.2F;

// Enemy spawn-rate cycling: the base interval steps down (rate up) once per
// 10-wave cycle; the back half of each cycle spawns faster than the front
// half.
constexpr int spawnRateCycleLength = 10;
constexpr float spawnRateCycleStep = 0.1F;
constexpr float spawnRateSecondHalfMultiplier = 0.6F;
constexpr float spawnIntervalFloor = 0.15F;

// Boss enrage: once a boss drops below enrageHealthFrac of its max HP, its
// idle cooldown and windup telegraphs speed up.
constexpr float enrageHealthFrac = 0.25F;
constexpr float enrageSpeedMult = 0.5F;

// Boss projectiles (Homing/Spread) pass through asteroids and enemies
// unharmed - only sustained player fire can shoot them down. Their HP scales
// with wave/difficulty like everything else so they stay a real threat.
constexpr int32_t baseProjectileHealth = 3;

// Wormhole mouths trigger this close; the pair spawns/despawns on a timer
// like the black hole, and each lives lifetimeSeconds once active.
constexpr float wormholeRadius = 26;
constexpr float wormholeLifetime = 20.0F;
constexpr float wormholeSpawnCooldownMin = 40.0F;
constexpr float wormholeSpawnCooldownMax = 70.0F;

// WormholeBeam: the boss fires its beam from a wormhole mouth flanking the
// player instead of from its own position.
constexpr float wormholeBeamDuration = 2.5F;
constexpr float wormholeBeamFlankMinDist = 150;
constexpr float wormholeBeamFlankMaxDist = 260;
constexpr float beamAttackDuration = 4.0F;

// Elite Hazards: rare, tough field hazards that hold near the screen edge
// rather than chasing. Concurrent count is capped by difficulty; HP sits
// strictly between a regular enemy and a miniboss.
constexpr float eliteHazardSpawnIntervalMin = 45.0F;
constexpr float eliteHazardSpawnIntervalMax = 90.0F;
constexpr float eliteHazardOrbitDistFrac = 0.85F; // fraction of the screen half-extent
constexpr float eliteHazardOrbitSpin = 6.0F;      // degrees/sec drift around the player
constexpr float eliteHazardFollowRate = 1.2F;     // position lerp speed toward its orbit point
constexpr int32_t eliteHazardBaseHealth = 220;    // above regular enemies, below a miniboss
constexpr int32_t eliteHazardScore = 150;
constexpr int32_t eliteHazardXpBonus = 300; // deliberately well above a regular enemy's XP drop
constexpr int32_t eliteHazardContactDamage = 2;
constexpr float warlordSpeedBuff = 1.35F;
constexpr float suppressorCooldownPenalty = 1.4F;

// New boss move constants (MineDrop/ChargeDash/SummonAdds/ShockwaveStomp/
// Barrage/GravityWell) - see startBossAttack/updateBoss for how each fires.
constexpr int mineDropCount = 4;
constexpr float mineDropRadius = 30;
constexpr int32_t mineDropDamage = 12;
// chargeDashSpeed/Duration: the visible world viewport is ~screenWidth x
// screenHeight world units (the pixelScale render-target downscale and the
// 1/pixelScale camera zoom cancel out - see beginWorldCamera), so this is
// tuned to close ~2200 units in half a second: a real edge-to-edge dash
// across the arena, not a small nudge.
constexpr float chargeDashSpeed = 4400;
constexpr float chargeDashDuration = 0.5F;
// SummonAdds spawns more adds on harder difficulties (Easy/Normal/Hard).
constexpr std::array<int, static_cast<size_t>(Difficulty::Count)> summonAddsCountByDifficulty{2, 3,
                                                                                              4};
constexpr float barrageFireInterval = 0.3F;
constexpr float barrageDuration = 2.5F;
// barrageProjSpeed must clear the player's own max move speed (game.player.speed
// * (1 + nerveSpeedBonusMax), i.e. up to 6/frame-equivalent) or a straight
// shot could never connect against a player retreating in a straight line.
constexpr float barrageProjSpeed = 8;
constexpr float homingBarrageFireInterval = 0.5F;

// Spread fires spreadRoundsByDifficulty(Easy/Normal/Hard) rounds of
// spreadBulletsPerRound each, spreadRoundInterval apart - a 10/30/60 total
// bullet-hell wall that scales with difficulty instead of one fixed fan.
constexpr int spreadBulletsPerRound = 10;
constexpr std::array<int, static_cast<size_t>(Difficulty::Count)> spreadRoundsByDifficulty{1, 3, 6};
constexpr float spreadRoundInterval = 0.25F;
constexpr float gravityWellPullStrength = 1.6F;
constexpr float gravityWellDuration = 2.0F;

constexpr std::array<float, static_cast<size_t>(WeaponType::Count)> weaponBaseCooldown{
    0.45F, 1.0F, 1.2F, 0.8F, 0.7F, 1.8F};

struct WaveHitResult
{
    bool resolved;
    bool hit;
};

void triggerShake(Game& game, float intensity, float duration);
void triggerHitPause(Game& game, float duration);
auto updateTitle(Game& game) -> bool;
auto updatePaused(Game& game) -> bool;
auto updateGameOver(Game& game) -> bool;
auto updateLevelUp(Game& game) -> bool;
void playMenuSounds(Game& game, int32_t newIndex, bool confirmed);
void playSFX(Game& game, Sound sound);
auto updateSettings(Game& game) -> bool;
void cycleResolution(Game& game, int32_t dir);
void cycleFPS(Game& game, int32_t dir);
void cycleDifficulty(Game& game, int32_t dir);
void gainNerve(Game& game);
void updateNerve(Game& game, float deltaTime);
void damagePlayer(Game& game, int32_t amount);
void spawnDeathExplosion(Game& game);
void updateDeathParticles(Game& game, float deltaTime);
void updateGameplay(Game& game, float deltaTime);
auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult;
void updateBossDeathShockwave(Game& game, float deltaTime);
void updatePlayerMovement(Game& game, float deltaTime);
void updateShieldAndBarrier(Game& game, float deltaTime);
void updateAbilityCharges(Game& game, float deltaTime);
auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>;
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float;
auto weaponDamage(const Game& game, int32_t level) -> int32_t;
auto mineCount(int32_t level, bool evolved) -> int;
auto mineRadius(bool evolved) -> float;
auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t;
void spawnMines(Game& game, const Weapon& weapon);
void updateMines(Game& game, float deltaTime);
auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
void updateWeapons(Game& game, float deltaTime);
void recordDamage(Game& game, DamageSource source, int32_t amount);
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source);
void beamPulse(Game& game, Vector2 dir, float length, int32_t dmg, DamageSource source);
void updateWaveSpawner(Game& game, float deltaTime);
auto asteroidIntervalMultiplier(const Game& game) -> float;
auto asteroidCap(const Game& game) -> int;
auto eliteHazardCap(const Game& game) -> int;
void updateEliteHazards(Game& game, float deltaTime);
void damageEliteHazard(Game& game, size_t index, int32_t amount);
auto enemyDamage(const Game& game, int32_t base) -> int32_t;
void spawnEnemy(Game& game);
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega);
auto bossMoveCountForDifficulty(Difficulty difficulty) -> int;
auto sampleBossMoveset(int count) -> std::vector<BossAttack>;
auto spawnRingPosition(const Game& game) -> Vector2;
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void updatePickups(Game& game, float deltaTime);
void collectLifeOrb(Game& game);
auto equippedSlotCount(const Game& game) -> int;
auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>;
auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>;
auto rollRewardChoices() -> std::vector<LevelUpChoice>;
auto weaponEvolved(const Game& game, WeaponType kind) -> bool;
void applySkill(Game& game, SkillType id);
void grantOrLevelWeapon(Game& game, WeaponType kind);
void applyEvolution(Game& game, WeaponType kind);
void updateBullets(Game& game, float deltaTime);
void damageEnemy(Game& game, size_t index, int32_t amount);
void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot);
void spawnPickup(Game& game, Vector2 position, int value, PickupType type);
void updateAsteroids(Game& game, float deltaTime);
void updateEnemies(Game& game, float deltaTime);
void updateProjectiles(Game& game, float deltaTime);
void filterDeadEntities(Game& game);
void updateBgParticles(Game& game);
void updateBlackHole(Game& game, float deltaTime);
void updateWormhole(Game& game, float deltaTime);
auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool;
void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter);
void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd);
void updateBgmLayers(Game& game, float deltaTime);

// triggerShake raises the screen-shake amplitude/duration to at least the
// given values (multiple simultaneous triggers don't stack, the strongest
// wins).
void triggerShake(Game& game, float intensity, float duration)
{
    if (intensity > game.shakeIntensity)
    {
        game.shakeIntensity = intensity;
    }
    if (duration > game.shakeTimer)
    {
        game.shakeTimer = duration;
        game.shakeDuration = duration;
    }
}

// triggerHitPause freezes gameplay for a brief moment to sell an impactful
// hit.
void triggerHitPause(Game& game, float duration)
{
    if (duration > game.hitPauseTimer)
    {
        game.hitPauseTimer = duration;
    }
}

auto updateTitle(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, 3, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
        MenuLayout::buttonGap, MenuLayout::titleMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        switch (index)
        {
        case 0:
            resetRun(game);
            break;
        case 1:
            game.settingsReturnState = GameState::TITLE;
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 2:
            return true;
        default:
            break;
        }
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
        game, game.menuIndex, 4, MenuLayout::buttonWidth, MenuLayout::buttonHeight,
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
            game.menuIndex = 0;
            game.state = GameState::SETTINGS;
            break;
        case 2:
            resetRun(game);
            break;
        case 3:
            return true;
        default:
            break;
        }
    }

    return false;
}

auto updateGameOver(Game& game) -> bool
{
    // Sandbox runs (unlimited spawns, god-mode toggle, forced boss attacks)
    // never touch the real high-score list - only a genuine playthrough can.
    if (!game.scoreRecorded && !game.sandbox)
    {
        game.highScores = highscore::record(*game.highScoreRepo, game.highScores, game.score);
        game.scoreRecorded = true;
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
            resetRun(game);
            break;
        case 1:
            return true;
        default:
            break;
        }
    }

    return false;
}

// updateLevelUp handles the non-cancelable skill/evolve picker that pauses
// the world when the player levels up.
auto updateLevelUp(Game& game) -> bool
{
    const auto [index, confirmed] = updateMenuSelectionWindow(
        game, game.menuIndex, static_cast<int32_t>(game.pendingChoices.size()),
        MenuLayout::levelUpWidth, MenuLayout::levelUpHeight, MenuLayout::levelUpGap,
        MenuLayout::levelUpMenuY);
    playMenuSounds(game, index, confirmed);
    game.menuIndex = index;

    if (confirmed)
    {
        const auto& choice = game.pendingChoices.at(static_cast<size_t>(index));
        switch (choice.type)
        {
        case ChoiceType::Evolve:
            applyEvolution(game, choice.weapon.value());
            break;
        case ChoiceType::LifeOrb:
            for (int32_t i = 0; i < choice.count; i++)
            {
                collectLifeOrb(game);
            }
            break;
        case ChoiceType::Shield:
            for (int32_t i = 0; i < choice.count; i++)
            {
                if (game.player.shieldStacks < playerConstants::maxShieldStack)
                {
                    game.player.shieldStacks++;
                }
            }
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
        playSFX(game, game.sounds.menuConfirm);
    }
    else if (newIndex != game.menuIndex)
    {
        playSFX(game, game.sounds.menuMove);
    }
}

// playSFX is the single path all sound-effect playback flows through, so the
// Sound on/off setting has one place to take effect.
void playSFX(Game& game, Sound sound)
{
    if (game.settings.soundOn)
    {
        PlaySound(sound);
    }
}

// updateSettings handles the settings screen: Up/Down selects a row,
// Left/Right (or Enter/Space) changes that row's value.
auto updateSettings(Game& game) -> bool
{
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
        game.menuIndex = settingsRowCount - 1;
    }
    if (game.menuIndex >= settingsRowCount)
    {
        game.menuIndex = 0;
    }

    const bool left = IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT);
    const bool right = IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT);
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);

    // Mouse hover only steals the row when the mouse actually moved this
    // frame - see updateMenuSelectionWindow for why (a keyboard Up/Down
    // press would otherwise get overwritten by the mouse sitting on the old
    // row).
    const Vector2 mouseDelta = GetMouseDelta();
    const bool mouseMoved = mouseDelta.x != 0 || mouseDelta.y != 0;
    if (mouseMoved)
    {
        if (const auto row = hoveredColumnRow(settingsRowCount, MenuLayout::settingsWidth,
                                              MenuLayout::settingsHeight, MenuLayout::settingsGap,
                                              MenuLayout::settingsMenuY, game);
            row.has_value())
        {
            game.menuIndex = *row;
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        if (const auto row = hoveredColumnRow(settingsRowCount, MenuLayout::settingsWidth,
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
        playSFX(game, game.sounds.menuMove);
    }

    switch (game.menuIndex)
    {
    case 0: // Resolution
        if (left)
        {
            cycleResolution(game, -1);
        }
        if (right || confirm)
        {
            cycleResolution(game, 1);
        }
        break;
    case 1: // Difficulty - locked mid-run: every difficulty-scaled formula
            // reads game.settings.difficulty live, so changing it from the
            // pause menu could otherwise be used to cash in Easy's
            // boss-attack loot exception for one hit, then flip back to
            // Hard's tougher stats immediately after.
        if (game.settingsReturnState != GameState::PAUSED)
        {
            if (left)
            {
                cycleDifficulty(game, -1);
            }
            if (right || confirm)
            {
                cycleDifficulty(game, 1);
            }
        }
        break;
    case 2: // BGM
        if (left || right || confirm)
        {
            game.settings.bgmOn = !game.settings.bgmOn;
            applyBGMState(game);
        }
        break;
    case 3: // Sound
        if (left || right || confirm)
        {
            game.settings.soundOn = !game.settings.soundOn;
        }
        break;
    case 4: // FPS cap
        if (left)
        {
            cycleFPS(game, -1);
        }
        if (right || confirm)
        {
            cycleFPS(game, 1);
        }
        break;
    case 5: // Display mode
        if (left || right || confirm)
        {
            toggleFullscreen(game);
        }
        break;
    case 6: // Back
        if (confirm)
        {
            game.state = game.settingsReturnState;
            game.menuIndex = 0;
        }
        break;
    default:
        break;
    }

    if ((left || right || confirm) && game.menuIndex <= 4)
    {
        saveSettings(game.settings);
    }

    return false;
}

void cycleResolution(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(resolutionOptions.size());
    game.settings.resolutionIndex = (game.settings.resolutionIndex + dir + n) % n;
    const auto& opt = resolutionOptions.at(static_cast<size_t>(game.settings.resolutionIndex));
    SetWindowSize(opt.width, opt.height);
    syncScreenSize(game);
}

void cycleFPS(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(fpsOptions.size());
    game.settings.fpsIndex = (game.settings.fpsIndex + dir + n) % n;
    SetTargetFPS(fpsOptions.at(static_cast<size_t>(game.settings.fpsIndex)));
}

void cycleDifficulty(Game& game, int32_t dir)
{
    const auto n = static_cast<int32_t>(Difficulty::Count);
    game.settings.difficulty =
        static_cast<Difficulty>((static_cast<int32_t>(game.settings.difficulty) + dir + n) % n);
}

void applyBGMState(Game& game)
{
    if (game.settings.bgmOn)
    {
        ResumeMusicStream(game.bgm.drone);
        ResumeMusicStream(game.bgm.intensity);
        ResumeMusicStream(game.bgm.upgrade);
    }
    else
    {
        PauseMusicStream(game.bgm.drone);
        PauseMusicStream(game.bgm.intensity);
        PauseMusicStream(game.bgm.upgrade);
    }
}

// updateBgmLayers fades the intensity layer toward the current enemy count
// and the upgrade layer in (permanently) once the player has any weapon
// upgrade. calmTimer briefly caps intensity after a boss kill - "the fight's
// not over, but take a breath" - unless enemies have already swarmed back to
// a heavy count, in which case the calm is cut short.
void updateBgmLayers(Game& game, float deltaTime)
{
    constexpr float intensityEnemyThreshold = 25.0F;
    constexpr float volumeSmoothing = 1.5F;
    constexpr float calmCeiling = 0.35F;
    constexpr float swarmOverrideThreshold = 0.7F;
    // A floor, not zero - with no enemies (title screen, a fresh run) the
    // drone alone is just a held chord with no rhythm; keeping the beat
    // layer audibly present at rest is what makes the score feel alive
    // instead of a single sustained note, rising further as enemies swarm.
    constexpr float intensityFloor = 0.3F;

    float intensityTarget = std::clamp(
        static_cast<float>(game.enemies.size()) / intensityEnemyThreshold, intensityFloor, 1.0F);

    if (game.bgm.calmTimer > 0)
    {
        if (intensityTarget >= swarmOverrideThreshold)
        {
            game.bgm.calmTimer = 0;
        }
        else
        {
            intensityTarget = std::min(intensityTarget, calmCeiling);
            game.bgm.calmTimer -= deltaTime;
        }
    }

    const bool hasWeaponUpgrade =
        game.weapons.size() > 1 ||
        std::ranges::any_of(game.weapons, [](const Weapon& w) { return w.level > 1; });
    const float upgradeTarget = hasWeaponUpgrade ? 1.0F : 0.0F;

    const float rate = 1 - std::exp(-volumeSmoothing * deltaTime);
    game.bgm.intensityVolume += (intensityTarget - game.bgm.intensityVolume) * rate;
    game.bgm.upgradeVolume += (upgradeTarget - game.bgm.upgradeVolume) * rate;

    SetMusicVolume(game.bgm.intensity, game.bgm.intensityVolume);
    SetMusicVolume(game.bgm.upgrade, game.bgm.upgradeVolume);
}

auto nerveFrac(const Game& game) -> float { return game.player.nerve / UpdateConstants::nerveMax; }

void gainNerve(Game& game)
{
    game.player.nerve += nerveKillGain;
    if (game.player.nerve > UpdateConstants::nerveMax)
    {
        game.player.nerve = UpdateConstants::nerveMax;
    }

    // Landing a kill also chips away at the ability-charge regen timer, so
    // staying aggressive earns dash/shield charges back faster than turtling
    // out the clock does.
    if (game.player.charges < UpdateConstants::maxCharges)
    {
        game.player.chargeRegenTimer -= 0.4F;
    }
}

void updateNerve(Game& game, float deltaTime)
{
    if (game.player.nerve > 0)
    {
        game.player.nerve -= nerveDecayPerSec * deltaTime;
        if (game.player.nerve < 0)
        {
            game.player.nerve = 0;
        }
    }
}

// damagePlayer is the single path all player damage flows through: it
// applies the hit and transitions to GAME_OVER if health runs out.
void damagePlayer(Game& game, int32_t amount)
{
    if (game.sandbox && !game.sandboxDeathEnabled)
    {
        return;
    }

    game.player.nerve = 0;

    if (game.player.shieldStacks > 0)
    {
        game.player.shieldStacks--;
        game.player.immunityTimer = 1.0F;
        playSFX(game, game.sounds.hit);
        triggerShake(game, 3, 0.15F);
        return;
    }

    game.player.health -= amount;
    game.player.immunityTimer = 1.0F;

    if (game.player.health <= 0)
    {
        game.state = GameState::GAME_OVER;
        game.menuIndex = 0;
        playSFX(game, game.sounds.defeat);
        triggerShake(game, 12, 0.5F);
        triggerHitPause(game, 0.15F);
        spawnDeathExplosion(game);
    }
    else
    {
        playSFX(game, game.sounds.hit);
        triggerShake(game, 4, 0.2F);
    }
}

// spawnDeathExplosion bursts debris outward from the player on death -
// purely cosmetic, kept animating independent of game.state (see
// updateDeathParticles's call site in UpdateGame) so it survives into the
// Game Over screen instead of freezing.
void spawnDeathExplosion(Game& game)
{
    const std::array<Color, 3> debrisColors{Palette::Accent, Palette::Crit, Palette::Haze};

    for (int i = 0; i < 28; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const float speed = static_cast<float>(GetRandomValue(20, 80)) / 10.0F;
        const Vector2 velocity{.x = std::cos(angle) * speed, .y = std::sin(angle) * speed};
        const float life = static_cast<float>(GetRandomValue(60, 100)) / 100.0F;

        game.deathParticles.push_back(
            Particle{.position = game.player.position,
                     .velocity = velocity,
                     .radius = static_cast<float>(GetRandomValue(2, 5)),
                     .life = life,
                     .maxLife = life,
                     .color = debrisColors.at(static_cast<size_t>(
                         GetRandomValue(0, static_cast<int32_t>(debrisColors.size() - 1))))});
    }
}

void updateDeathParticles(Game& game, float deltaTime)
{
    for (auto& p : game.deathParticles)
    {
        p.position = Vector2Add(p.position, p.velocity);
        p.life -= deltaTime;
    }

    std::erase_if(game.deathParticles, [](const Particle& p) { return p.life <= 0; });
}

void updateGameplay(Game& game, float deltaTime)
{
    // The damage meter is this frame's damage only - reset before anything
    // this frame has a chance to deal any, so a frame with no hits reads 0.
    game.damageMeter = DamageMeter{};

    if (IsKeyPressed(KEY_ESCAPE))
    {
        game.state = GameState::PAUSED;
        game.menuIndex = 0;
        return;
    }

    game.runTime += deltaTime;

    if (game.sandbox)
    {
        updateSandboxInput(game);
    }

    updateNerve(game, deltaTime);
    updateAbilityCharges(game, deltaTime);
    updatePlayerMovement(game, deltaTime);
    updateShieldAndBarrier(game, deltaTime);
    updateWeapons(game, deltaTime);
    updateWaveSpawner(game, deltaTime);
    updateBlackHole(game, deltaTime);
    updateWormhole(game, deltaTime);
    updateEliteHazards(game, deltaTime);

    if (game.player.health > 0 && game.blackhole.active &&
        Vector2Distance(game.player.position, game.blackhole.position) <= game.blackhole.radius)
    {
        game.player.blackHoleCoreTimer += deltaTime;
        if (game.player.blackHoleCoreTimer >= 1.0F)
        {
            game.player.blackHoleCoreTimer = 0;
            damagePlayer(game, game.player.health);
        }
    }
    else
    {
        game.player.blackHoleCoreTimer = 0;
    }

    for (auto& boss : game.bosses)
    {
        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};
        updateBossMovement(game, deltaTime, boss, bossCenter);
        updateBoss(game, deltaTime, boss, bossCenter);
    }

    updateBullets(game, deltaTime);
    updateAsteroids(game, deltaTime);
    updateEnemies(game, deltaTime);
    updateProjectiles(game, deltaTime);
    updateMines(game, deltaTime);
    updatePickups(game, deltaTime);

    filterDeadEntities(game);

    // Every boss that dropped to 0 HP this frame gets its own death
    // shockwave and reward - bosses persist independently, so a fresh spawn
    // never displaces one that's still fighting, and simultaneous kills each
    // get their own effects rather than clobbering one another.
    bool anyBossKilledThisFrame = false;
    for (auto& boss : game.bosses)
    {
        if (boss.health > 0)
        {
            continue;
        }

        const Vector2 bossCenter{.x = boss.position.x + boss.size.x / 2,
                                 .y = boss.position.y + boss.size.y / 2};

        boss.health = 0;
        boss.color = Palette::StructDark;
        game.score += 1000;
        anyBossKilledThisFrame = true;

        // Boss kill reward: miniboss XP guarantees a level-up (with a bit of
        // leftover as a felt "boost"); megaboss XP overshoots enough to
        // usually chain a second level-up once xpToNext recalculates next
        // frame. A swarm kill also drops a guaranteed jackpot pickup.
        game.xp += static_cast<int>(static_cast<float>(game.xpToNext) *
                                     (boss.isMega ? megaBossXpMult : miniBossXpMult));
        if (boss.isSwarm)
        {
            spawnPickup(game, bossCenter, 0, PickupType::Shield);
            spawnPickup(game, bossCenter, 0, PickupType::LifeOrb);
        }

        game.bossDeathShockwaves.push_back(
            BossDeathShockwave{.timer = UpdateConstants::bossDeathShockwaveDuration,
                               .position = bossCenter,
                               .hit = false});

        if (game.player.health > 0)
        {
            playSFX(game, game.sounds.victory);
            triggerShake(game, 14, 0.6F);
            triggerHitPause(game, 0.2F);
            gainNerve(game);
        }
    }
    if (anyBossKilledThisFrame)
    {
        game.bgm.calmTimer = bossKillCalmDuration;
    }
    std::erase_if(game.bosses, [](const Boss& boss) { return boss.health <= 0; });

    updateBossDeathShockwave(game, deltaTime);

    if (game.xp >= game.xpToNext)
    {
        startLevelUp(game);
    }

    // Damage meter display: hold the last nonzero reading for a bit instead
    // of flickering blank the instant a frame doesn't land a hit, so numbers
    // are actually readable rather than a single-frame flash.
    if (game.damageMeter.total > 0)
    {
        game.damageMeterDisplay = game.damageMeter;
        game.damageMeterHoldTimer = damageMeterHoldDuration;
    }
    else if (game.damageMeterHoldTimer > 0)
    {
        game.damageMeterHoldTimer -= deltaTime;
        if (game.damageMeterHoldTimer <= 0)
        {
            game.damageMeterDisplay = DamageMeter{};
        }
    }
}

// resolveExpandingWaveHit checks a slow-expanding hazard (boss Slam / death
// shockwave) against the player once per frame: if they're within the
// hazard's eventual max radius and have a shield bubble, a shield stack, or
// (if allowDash) an active dash up at any point before the growing radius
// physically reaches them, that counts as dodging it - consuming a shield
// stack if that's specifically what protected them. Only once the radius
// itself crosses the player unprotected does it report a hit. A transient
// post-hit immunityTimer deliberately does NOT count as protection here - it
// has nothing to do with dodging this specific hazard, and treating it as a
// free pass let an unrelated graze on the way in cancel the hazard entirely.
auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult
{
    const float distToPlayer = Vector2Distance(from, game.player.position);
    const bool inDanger = distToPlayer <= bossConstants::maxSlamRadius + game.player.radius;
    const bool dashProtected = allowDash && game.player.dashing;

    if (inDanger && (game.player.shieldActive || dashProtected || game.player.shieldStacks > 0))
    {
        if (!game.player.shieldActive && !dashProtected && game.player.shieldStacks > 0)
        {
            game.player.shieldStacks--;
        }
        return WaveHitResult{.resolved = true, .hit = false};
    }

    if (distToPlayer <= radius + game.player.radius)
    {
        return WaveHitResult{.resolved = true, .hit = true};
    }

    return WaveHitResult{.resolved = false, .hit = false};
}

// updateBossDeathShockwave expands a one-time ring from the boss's death
// position, destroying every enemy/asteroid it passes over and damaging the
// player once if it reaches them and they're not shielded. Unlike a boss's
// regular attacks (which deny loot on Normal/Hard via killEnemyForBossAttack's
// alwaysLoot=false), this always pays out score/XP/pickups regardless of
// difficulty - the "cash-in" reward for actually finishing the boss.
void updateBossDeathShockwave(Game& game, float deltaTime)
{
    for (auto& wave : game.bossDeathShockwaves)
    {
        wave.timer -= deltaTime;

        const float progress =
            std::clamp(1 - wave.timer / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float radius = bossConstants::maxSlamRadius * progress;

        for (size_t i = 0; i < game.enemies.size(); i++)
        {
            if (game.enemies.at(i).active &&
                Vector2Distance(wave.position, game.enemies.at(i).position) <= radius)
            {
                killEnemyForBossAttack(game, i, true);
            }
        }

        for (auto& asteroid : game.asteroids)
        {
            if (asteroid.active && Vector2Distance(wave.position, asteroid.position) <= radius)
            {
                asteroid.active = false;
            }
        }

        // A surviving boss caught in another boss's death ring only takes a
        // one-time chip of damage as the ring passes through it (detected by
        // the crossing between last frame's and this frame's radius) - a
        // flinch, not a way to cascade-kill a multi-boss fight for free.
        const float prevProgress = std::clamp(
            1 - (wave.timer + deltaTime) / UpdateConstants::bossDeathShockwaveDuration, 0.0F, 1.0F);
        const float prevRadius = bossConstants::maxSlamRadius * prevProgress;
        for (auto& other : game.bosses)
        {
            const Vector2 otherCenter{.x = other.position.x + other.size.x / 2,
                                      .y = other.position.y + other.size.y / 2};
            const float distToBoss = Vector2Distance(wave.position, otherCenter);
            if (distToBoss <= radius && distToBoss > prevRadius)
            {
                other.health -= std::max(1, other.maxHealth / 50);
            }
        }

        if (!wave.hit)
        {
            if (const auto result = resolveExpandingWaveHit(game, wave.position, radius, false);
                result.resolved)
            {
                wave.hit = true;
                if (result.hit)
                {
                    damagePlayer(game, enemyDamage(game, 1));
                }
            }
        }
    }

    std::erase_if(game.bossDeathShockwaves,
                  [](const BossDeathShockwave& wave) { return wave.timer <= 0; });
}

void updatePlayerMovement(Game& game, float deltaTime)
{
    const bool inBlackHole =
        game.blackhole.active && Vector2Distance(game.player.position, game.blackhole.position) <=
                                     game.blackhole.influenceRadius;

    if (game.player.slowTimer > 0)
    {
        game.player.slowTimer -= deltaTime;
    }

    if (game.player.dashing)
    {
        game.player.dashTimer -= deltaTime;
        if (game.player.dashTimer <= 0)
        {
            game.player.dashing = false;
        }
        game.player.position = Vector2Add(
            game.player.position, Vector2Scale(game.player.dashVelocity, deltaTime * frameScale));
    }
    else
    {
        float effectiveSpeed = game.player.speed * (1 + nerveSpeedBonusMax * nerveFrac(game));
        if (inBlackHole)
        {
            effectiveSpeed -= blackHoleSlow;
        }
        if (game.player.slowTimer > 0)
        {
            effectiveSpeed *= 0.5F;
        }
        if (effectiveSpeed < 1)
        {
            effectiveSpeed = 1;
        }

        Vector2 moveDelta{};

        if (IsKeyDown(KEY_D))
        {
            moveDelta.x += effectiveSpeed;
        }
        if (IsKeyDown(KEY_A))
        {
            moveDelta.x -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_W))
        {
            moveDelta.y -= effectiveSpeed;
        }
        if (IsKeyDown(KEY_S))
        {
            moveDelta.y += effectiveSpeed;
        }

        game.player.position =
            Vector2Add(game.player.position, Vector2Scale(moveDelta, deltaTime * frameScale));
    }

    if (inBlackHole)
    {
        const Vector2 toHole = Vector2Subtract(game.blackhole.position, game.player.position);
        if (Vector2Length(toHole) > 0)
        {
            const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHolePull);
            game.player.position =
                Vector2Add(game.player.position, Vector2Scale(pull, deltaTime * frameScale));
        }
    }

    if (game.player.dashing)
    {
        applyWormholeTransit(game, game.player.position, game.player.dashVelocity,
                             game.player.radius);
    }
    else
    {
        Vector2 noVelocity{};
        applyWormholeTransit(game, game.player.position, noVelocity, game.player.radius);
    }

    if (const float dist = Vector2Length(game.player.position); dist > GameConstants::arenaHalf)
    {
        game.player.position =
            Vector2Scale(Vector2Normalize(game.player.position), GameConstants::arenaHalf);
    }

    // Standing inside a boss's body is not an instant hit: dashing through it
    // (Shield+Dash hardest) rams and punishes the boss for free, but
    // lingering there un-dashed for too long gets you destroyed - the same
    // "get in, get out" tension as the black hole core, just with a boss
    // that fights back if you commit to the hit.
    bool touchingAnyBoss = false;
    for (auto& boss : game.bosses)
    {
        const Rectangle bossRect{.x = boss.position.x,
                                 .y = boss.position.y,
                                 .width = boss.size.x,
                                 .height = boss.size.y};
        if (boss.health <= 0 ||
            !CheckCollisionCircleRec(game.player.position, game.player.radius, bossRect))
        {
            continue;
        }

        touchingAnyBoss = true;

        if (game.player.dashing && !boss.hitByDash)
        {
            boss.hitByDash = true;
            const int32_t ramDamage = game.player.shieldActive ? bossRamDamage * 2 : bossRamDamage;
            boss.health -= ramDamage;
            triggerShake(game, 14, 0.4F);
            triggerHitPause(game, 0.12F);
            if (game.player.shieldActive)
            {
                game.player.chargeRegenTimer =
                    std::max(0.0F, game.player.chargeRegenTimer - shieldKillChargeRefund);
            }
        }
    }

    if (touchingAnyBoss && !game.player.dashing)
    {
        game.player.bossBodyTimer += deltaTime;
        if (game.player.bossBodyTimer >= bossBodyLingerLimit)
        {
            game.player.bossBodyTimer = 0;
            damagePlayer(game, game.player.health);
        }
    }
    else
    {
        game.player.bossBodyTimer = 0;
    }

    if (game.player.immunityTimer > 0)
    {
        game.player.immunityTimer -= deltaTime;
    }
}

// updateShieldAndBarrier counts down the shield's active/cooldown timers.
// Activation itself is manual (right-click, see updateAbilityCharges).
void updateShieldAndBarrier(Game& game, float deltaTime)
{
    if (game.player.shieldCooldownTimer > 0)
    {
        game.player.shieldCooldownTimer -= deltaTime;
    }

    if (game.player.shieldActive)
    {
        game.player.shieldTimer -= deltaTime;

        if (game.player.shieldTimer <= 0)
        {
            game.player.shieldActive = false;
            game.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
        }
    }
}

// chargeRegenDuration is how long one charge takes to regenerate, sped up by
// the Barrier Mastery skill.
auto chargeRegenDuration(const Game& game) -> float
{
    const float duration =
        UpdateConstants::chargeRegenTime -
        static_cast<float>(game.skillLevels.at(static_cast<size_t>(SkillType::Barrier))) * 0.3F;
    return duration < 1 ? 1 : duration;
}

// updateAbilityCharges regenerates the shared 2-charge pool and handles the
// left-click dash / right-click shield triggers, each spending one charge.
void updateAbilityCharges(Game& game, float deltaTime)
{
    if (game.player.charges < UpdateConstants::maxCharges)
    {
        game.player.chargeRegenTimer -= deltaTime;
        if (game.player.chargeRegenTimer <= 0)
        {
            game.player.charges++;
            game.player.chargeRegenTimer = chargeRegenDuration(game);
        }
    }

    // Snapshotting charges here (rather than re-reading live game.player.charges
    // in each block below) means the emergency Nerve fallback only kicks in
    // for an ability that was ALREADY out of charges at the start of this
    // frame - not one that's out of charges only because the other ability
    // (dash/shield) just spent the last one earlier in this same frame. Without
    // this, clicking both buttons in one frame with 1 charge + full Nerve
    // banked would spend the 1 real charge on the first ability and get the
    // second for free off the emergency reserve, turning a rare safety net
    // into a routine 2-for-1.
    const int32_t chargesAtFrameStart = game.player.charges;

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !game.player.dashing)
    {
        const bool hasCharge = game.player.charges > 0;
        const bool emergency =
            chargesAtFrameStart == 0 &&
            game.player.nerve >= UpdateConstants::nerveMax * emergencyActionNerveThreshold;
        if (hasCharge || emergency)
        {
            // Out of charges: burn your whole built-up Nerve for an
            // emergency dash instead - real mobility when you need it, at
            // the cost of the damage/speed bonus you'd built up. Only
            // available once Nerve is mostly full (>=70%), so it's a
            // deliberate reserve to cash in, not a routine substitute.
            if (hasCharge)
            {
                game.player.charges--;
            }
            else
            {
                game.player.nerve = 0;
            }

            game.player.dashing = true;
            game.player.dashTimer = dashDuration;
            game.player.dashVelocity = Vector2Scale(aimAtMouse(game), dashSpeed);

            for (auto& enemy : game.enemies)
            {
                enemy.hitByDash = false;
            }
            for (auto& boss : game.bosses)
            {
                boss.hitByDash = false;
            }
        }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !game.player.shieldActive)
    {
        const bool hasCharge = game.player.charges > 0;
        const bool emergency =
            chargesAtFrameStart == 0 &&
            game.player.nerve >= UpdateConstants::nerveMax * emergencyActionNerveThreshold;
        if (hasCharge || emergency)
        {
            if (hasCharge)
            {
                game.player.charges--;
            }
            else
            {
                game.player.nerve = 0;
            }

            game.player.shieldActive = true;
            game.player.shieldTimer =
                shieldBaseDuration +
                static_cast<float>(game.skillLevels.at(static_cast<size_t>(SkillType::Barrier))) *
                    0.4F;
        }
    }
}

// aimAtMouse returns the normalized direction from screen-center (where the
// player always renders) to the current mouse position.
auto aimAtMouse(const Game& game) -> Vector2
{
    const Vector2 mouse = GetMousePosition();
    const Vector2 center{.x = static_cast<float>(game.windowWidth) / 2,
                         .y = static_cast<float>(game.windowHeight) / 2};
    const Vector2 dir = Vector2Subtract(mouse, center);
    if (Vector2Length(dir) == 0)
    {
        return Vector2{.x = 0, .y = -1};
    }
    return Vector2Normalize(dir);
}

// nearestEnemy is the shared "what should this home in on" target picker for
// homing missiles, mine seeking, etc. - considers both regular enemies and
// elite hazards (Warlord/Suppressor), which are otherwise valid, damageable
// targets (see aoePulse/beamPulse/updateProjectiles) but a separate entity
// list from game.enemies.
auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& enemy : game.enemies)
    {
        if (!enemy.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, enemy.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = enemy.position;
            found = true;
        }
    }

    for (const auto& hazard : game.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, hazard.position);
        if (best < 0 || d < best)
        {
            best = d;
            target = hazard.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

auto orbitRadius(int32_t level) -> float { return 55 + static_cast<float>(level) * 4; }

auto shockwaveRadius(int32_t level, bool evolved) -> float
{
    float radius = 90 + static_cast<float>(level) * 8;
    if (evolved)
    {
        radius *= 1.4F;
    }
    return radius;
}

// weaponCooldown returns the fire interval for a weapon kind at a given
// level, scaled by the Overclock passive.
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float
{
    float base = weaponBaseCooldown.at(static_cast<size_t>(kind));
    base -= static_cast<float>(level) * 0.02F;
    if (base < 0.12F)
    {
        base = 0.12F;
    }
    base *= 1 - 0.1F * static_cast<float>(
                           game.skillLevels.at(static_cast<size_t>(SkillType::Cooldown)));
    if (evolved)
    {
        base *= 0.8F;
    }

    // A Suppressor's debuff applies to the player across the whole screen,
    // not just within EliteHazardConstants::auraRadius - the aura ring drawn
    // around it (see drawEliteHazard) is a visual read on which hazard is
    // doing it, not a range gate.
    for (const auto& hazard : game.eliteHazards)
    {
        if (hazard.active && hazard.role == EliteHazardRole::Suppressor)
        {
            base *= suppressorCooldownPenalty;
            break;
        }
    }

    if (base < 0.08F)
    {
        base = 0.08F;
    }
    return base;
}

auto weaponDamage(const Game& game, int32_t level) -> int32_t
{
    auto dmg = static_cast<float>(baseBulletDamage + level * 2);
    dmg *=
        1 + 0.15F * static_cast<float>(game.skillLevels.at(static_cast<size_t>(SkillType::Damage)));
    dmg *= 1 + nerveDamageBonusMax * nerveFrac(game);
    dmg *= 1 + postCapDamageBonusPerLevel * static_cast<float>(game.postCapDamageLevels);
    return static_cast<int32_t>(dmg);
}

auto mineCount(int32_t level, bool evolved) -> int
{
    int count = 2 + static_cast<int>(level) / 3;
    if (evolved)
    {
        count++;
    }
    return count;
}

auto mineRadius(bool evolved) -> float { return evolved ? 42.0F : 32.0F; }

auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t
{
    int32_t dmg = weaponDamage(game, level);
    if (evolved)
    {
        dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
    }
    return dmg;
}

// spawnMines scatters mineCount mines at random offsets around the player;
// they home toward the nearest enemy/asteroid (see updateMines) rather than
// detonating immediately.
void spawnMines(Game& game, const Weapon& weapon)
{
    const int count = mineCount(weapon.level, weapon.evolved);
    const float radius = mineRadius(weapon.evolved);
    const int32_t dmg = mineDamage(game, weapon.level, weapon.evolved);

    for (int i = 0; i < count; i++)
    {
        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto dist = static_cast<float>(GetRandomValue(20, 50));
        const Vector2 pos = Vector2Add(game.player.position, Vector2{.x = std::cos(angle) * dist,
                                                                     .y = std::sin(angle) * dist});

        game.mines.push_back(Mine{.position = pos,
                                  .velocity = Vector2{},
                                  .fuse = mineLifetime,
                                  .radius = radius,
                                  .damage = dmg,
                                  .active = true,
                                  .evolved = weapon.evolved});
    }
}

auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    if (const auto target = nearestEnemy(game, from);
        target.has_value() && Vector2Distance(from, *target) <= maxDist)
    {
        return target;
    }
    return std::nullopt;
}

auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>
{
    float best = -1;
    Vector2 target{};
    bool found = false;

    for (const auto& asteroid : game.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }
        const float d = Vector2Distance(from, asteroid.position);
        if (d <= maxDist && (best < 0 || d < best))
        {
            best = d;
            target = asteroid.position;
            found = true;
        }
    }

    if (!found)
    {
        return std::nullopt;
    }
    return target;
}

// updateMines detonates each mine (an aoePulse at the mine's own position,
// hitting enemies, elite hazards, and asteroids alike) the instant something
// enters its blast ring (mine.radius - the same ring drawn around it, see
// drawGameplayWorld) or once its fuse runs out. Base (non-evolved) mines
// stay exactly where they were dropped (see spawnMines) and just wait for
// something to wander into the ring; evolved mines actively home in on the
// nearest enemy/hazard (or asteroid, if none in range) within mineSeekRadius
// until it's inside the ring.
void updateMines(Game& game, float deltaTime)
{
    for (auto& mine : game.mines)
    {
        if (!mine.active)
        {
            continue;
        }

        bool detonate = false;

        if (mine.evolved)
        {
            auto target = nearestEnemyWithin(game, mine.position, mineSeekRadius);
            if (!target.has_value())
            {
                target = nearestAsteroidWithin(game, mine.position, mineSeekRadius);
            }

            if (target.has_value())
            {
                const Vector2 dir = Vector2Subtract(*target, mine.position);
                if (Vector2Length(dir) <= mine.radius)
                {
                    detonate = true;
                }
                else
                {
                    mine.velocity = Vector2Scale(Vector2Normalize(dir), mineHomeSpeed);
                }
            }
            else
            {
                mine.velocity = Vector2{};
            }

            mine.position =
                Vector2Add(mine.position, Vector2Scale(mine.velocity, deltaTime * frameScale));
        }
        else if (nearestEnemyWithin(game, mine.position, mine.radius).has_value())
        {
            // Base mines don't chase - they sit exactly where they were
            // dropped (see spawnMines) and only trigger as a passive
            // proximity mine, going off the instant anything (enemy or
            // elite hazard) enters the blast ring drawn around it (see
            // drawGameplayWorld) - the ring is the actual trigger, not just
            // a decoration.
            detonate = true;
        }

        mine.fuse -= deltaTime;
        if (mine.fuse <= 0)
        {
            detonate = true;
        }

        if (detonate)
        {
            mine.active = false;
            aoePulse(game, mine.position, mine.radius, mine.damage, DamageSource::Mine);
        }
    }
}

// updateWeapons auto-fires every equipped weapon on its own cooldown - there
// is no manual fire key, matching the bullet-heaven auto-combat feel.
void updateWeapons(Game& game, float deltaTime)
{
    for (auto& w : game.weapons)
    {
        if (w.flashTimer > 0)
        {
            w.flashTimer -= deltaTime;
        }

        w.timer -= deltaTime;
        if (w.timer > 0)
        {
            continue;
        }
        w.timer = weaponCooldown(game, w.type, w.level, w.evolved);

        switch (w.type)
        {
        case WeaponType::Forward:
        {
            const Vector2 dir = aimAtMouse(game);
            int32_t shots = 1 + w.level / 3;
            if (w.evolved)
            {
                shots++;
            }
            constexpr float spread = 10;
            int32_t dmg = weaponDamage(game, w.level);
            Color color = Palette::Accent;
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                color = Palette::Crit;
            }
            for (int32_t s = 0; s < shots; s++)
            {
                const float angleOffset =
                    (static_cast<float>(s) - static_cast<float>(shots - 1) / 2) * spread * DEG2RAD;
                const float cosA = std::cos(angleOffset);
                const float sinA = std::sin(angleOffset);
                const Vector2 rotated{.x = dir.x * cosA - dir.y * sinA,
                                      .y = dir.x * sinA + dir.y * cosA};
                game.bullets.push_back(Bullet{.position = game.player.position,
                                              .velocity = Vector2Scale(rotated, projectileSpeed),
                                              .radius = projectileSize,
                                              .color = color,
                                              .active = true,
                                              .damage = dmg});
            }
            playSFX(game, game.sounds.shoot);
            break;
        }
        case WeaponType::Homing:
        {
            const int missiles = w.evolved ? 2 : 1;
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            bool fired = false;
            for (int m = 0; m < missiles; m++)
            {
                if (const auto target = nearestEnemy(game, game.player.position);
                    target.has_value())
                {
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(*target, game.player.position));
                    game.bossProjectiles.push_back(
                        BossProjectile{.position = game.player.position,
                                       .velocity = Vector2Scale(dir, homingProjSpeed * 1.5F),
                                       .radius = 6,
                                       .homing = true,
                                       .active = true,
                                       .fromPlayer = true,
                                       .damage = dmg});
                    fired = true;
                }
            }
            if (fired)
            {
                playSFX(game, game.sounds.homingLaunch);
            }
            break;
        }
        case WeaponType::Orbit:
        {
            float radius = orbitRadius(w.level);
            int32_t dmg = weaponDamage(game, w.level) / 2;
            if (w.evolved)
            {
                radius *= 1.4F;
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            aoePulse(game, game.player.position, radius, dmg, DamageSource::Orbit);
            break;
        }
        case WeaponType::Shock:
        {
            const float radius = shockwaveRadius(w.level, w.evolved);
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
                if (game.player.health < game.player.maxHealth)
                {
                    game.player.health++;
                }
            }
            aoePulse(game, game.player.position, radius, dmg, DamageSource::Shock);
            w.flashTimer = UpdateConstants::shockFlashDuration;
            break;
        }
        case WeaponType::Mine:
            spawnMines(game, w);
            break;
        case WeaponType::Beam:
        {
            const Vector2 dir = aimAtMouse(game);
            float length = 300 + static_cast<float>(w.level) * 15;
            int32_t dmg = weaponDamage(game, w.level);
            if (w.evolved)
            {
                length *= 1.3F;
                dmg = static_cast<int32_t>(static_cast<float>(dmg) * 1.5F);
            }
            beamPulse(game, dir, length, dmg, DamageSource::Beam);
            break;
        }
        case WeaponType::Count:
            break;
        }
    }
}

// recordDamage feeds the live damage meter (see DamageMeter) - this frame's
// outgoing damage only, broken down by source.
void recordDamage(Game& game, DamageSource source, int32_t amount)
{
    game.damageMeter.bySource.at(static_cast<size_t>(source)) += amount;
    game.damageMeter.total += amount;
}

// aoePulse damages/destroys everything within radius of the player in one
// instantaneous pulse - shared by Orbit, Shockwave, and Mine Layer, which
// differ only in radius/damage/cooldown numbers and visuals.
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source)
{
    bool hitAny = false;

    for (size_t j = 0; j < game.enemies.size(); j++)
    {
        const auto& enemy = game.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            Vector2Distance(center, enemy.position) <=
                radius + enemyKinds.at(static_cast<size_t>(enemy.kind)).radius)
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (size_t j = 0; j < game.eliteHazards.size(); j++)
    {
        const auto& hazard = game.eliteHazards.at(j);
        if (hazard.active &&
            Vector2Distance(center, hazard.position) <= radius + EliteHazardConstants::radius)
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (auto& asteroid : game.asteroids)
    {
        if (asteroid.active &&
            Vector2Distance(center, asteroid.position) <= radius + asteroid.radius)
        {
            asteroid.active = false;
            game.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.asteroids, asteroid);
            hitAny = true;
        }
    }

    if (hitAny)
    {
        playSFX(game, game.sounds.explosion);
    }
}

// beamPulse damages everything along a line from the player toward dir, out
// to length - Beam Sweep's attack.
void beamPulse(Game& game, Vector2 dir, float length, int32_t dmg, DamageSource source)
{
    const Vector2 start = game.player.position;
    const Vector2 end = Vector2Add(start, Vector2Scale(dir, length));
    bool hitAny = false;

    for (size_t j = 0; j < game.enemies.size(); j++)
    {
        const auto& enemy = game.enemies.at(j);
        if (enemy.active && !enemy.phased &&
            CheckCollisionCircleLine(
                enemy.position, enemyKinds.at(static_cast<size_t>(enemy.kind)).radius, start, end))
        {
            damageEnemy(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (size_t j = 0; j < game.eliteHazards.size(); j++)
    {
        const auto& hazard = game.eliteHazards.at(j);
        if (hazard.active &&
            CheckCollisionCircleLine(hazard.position, EliteHazardConstants::radius, start, end))
        {
            damageEliteHazard(game, j, dmg);
            recordDamage(game, source, dmg);
            hitAny = true;
        }
    }

    for (auto& asteroid : game.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, start, end))
        {
            asteroid.active = false;
            game.score += asteroidScore(asteroid.tier);
            breakAsteroid(game.asteroids, asteroid);
            hitAny = true;
        }
    }

    if (hitAny)
    {
        playSFX(game, game.sounds.explosion);
    }
}

// updateWaveSpawner escalates wave number/spawn rate over time and spawns
// enemies from the roster on a ring just outside the screen around the
// player; every 5th wave also brings the boss in.
void updateWaveSpawner(Game& game, float deltaTime)
{
    if (game.sandbox)
    {
        return;
    }

    game.waveTimer -= deltaTime;
    if (game.waveTimer <= 0)
    {
        game.waveNumber++;
        game.waveTimer = GameConstants::waveDuration;

        if (game.waveNumber % megaBossWaveInterval == 0)
        {
            spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true);
        }
        else if (game.waveNumber % miniBossWaveInterval == 0)
        {
            spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false);
        }
    }

    game.enemySpawnTimer -= deltaTime;
    if (game.enemySpawnTimer <= 0 &&
        static_cast<int>(game.enemies.size()) < UpdateConstants::maxEnemies)
    {
        // The base spawn rate steps up once per 10-wave cycle; within a
        // cycle, the first half spawns at that steady rate and the second
        // half spawns faster.
        const int cycleIndex = (game.waveNumber - 1) / spawnRateCycleLength;
        const int cyclePosition = (game.waveNumber - 1) % spawnRateCycleLength + 1; // 1..10

        const float baseInterval = std::max(
            1.2F - static_cast<float>(cycleIndex) * spawnRateCycleStep, spawnIntervalFloor);
        const float phaseMultiplier =
            cyclePosition <= spawnRateCycleLength / 2 ? 1.0F : spawnRateSecondHalfMultiplier;
        const float interval = std::max(baseInterval * phaseMultiplier, spawnIntervalFloor);

        game.enemySpawnTimer =
            interval *
            difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).spawnRateMult;

        const int spawnCount = 1 + game.waveNumber / 4;
        for (int i = 0;
             i < spawnCount && static_cast<int>(game.enemies.size()) < UpdateConstants::maxEnemies;
             i++)
        {
            spawnEnemy(game);
        }
    }

    const bool asteroidsUnlocked =
        game.settings.difficulty != Difficulty::Easy || game.waveNumber >= 10;

    game.asteroidSpawnTimer -= deltaTime;
    if (asteroidsUnlocked && game.asteroidSpawnTimer <= 0 &&
        static_cast<int>(game.asteroids.size()) < asteroidCap(game))
    {
        game.asteroidSpawnTimer =
            static_cast<float>(GetRandomValue(8, 16)) / 10.0F * asteroidIntervalMultiplier(game);

        AsteroidTier tier = AsteroidTier::Large;
        if (GetRandomValue(0, 1) == 1)
        {
            tier = AsteroidTier::Medium;
        }

        const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const Vector2 spawnPos = Vector2Add(
            game.player.position, Vector2{.x = std::cos(angle) * 500, .y = std::sin(angle) * 500});
        const Vector2 aimPoint = Vector2Add(
            game.player.position, Vector2{.x = static_cast<float>(GetRandomValue(-180, 180)),
                                          .y = static_cast<float>(GetRandomValue(-180, 180))});
        const Vector2 direction = Vector2Normalize(Vector2Subtract(aimPoint, spawnPos));
        const float speed = static_cast<float>(GetRandomValue(25, 45)) / 10.0F;

        game.asteroids.push_back(Asteroid{.position = spawnPos,
                                          .velocity = Vector2Scale(direction, speed),
                                          .radius = asteroidRadius(tier),
                                          .tier = tier,
                                          .active = true});
    }
}

// asteroidIntervalMultiplier/asteroidCap tune how annoying the asteroid
// field is per difficulty: Hard keeps today's baseline pace, Normal/Easy
// spawn slower and cap lower; Easy is additionally locked out entirely
// before wave 10 (see asteroidsUnlocked above).
auto asteroidIntervalMultiplier(const Game& game) -> float
{
    return game.settings.difficulty == Difficulty::Hard ? 1.0F : 1.6F;
}

auto asteroidCap(const Game& game) -> int
{
    switch (game.settings.difficulty)
    {
    case Difficulty::Hard:
        return maxAsteroid;
    case Difficulty::Easy:
        return maxAsteroid / 3;
    default:
        return maxAsteroid * 2 / 3;
    }
}

auto eliteHazardCap(const Game& game) -> int
{
    switch (game.settings.difficulty)
    {
    case Difficulty::Easy:
        return 1;
    case Difficulty::Hard:
        return 3;
    default:
        return 2;
    }
}

auto waveEnemyScale(const Game& game) -> float
{
    return 1 + static_cast<float>(game.waveNumber - 1) * UpdateConstants::waveEnemyScalePerWave;
}

// enemyDamage scales a base enemy damage amount by the difficulty's damage
// multiplier (Easy hits softer, Hard hits harder) and the wave's continuous
// scale-up.
auto enemyDamage(const Game& game, int32_t base) -> int32_t
{
    return static_cast<int32_t>(
        static_cast<float>(base) *
        difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyDamageMult *
        waveEnemyScale(game));
}

// spawnEnemy picks a weighted-random kind (eligible for the current wave)
// and places it just outside the screen around the player.
void spawnEnemy(Game& game)
{
    std::vector<int> eligible;
    eligible.reserve(enemyKinds.size());
    for (size_t i = 0; i < enemyKinds.size(); i++)
    {
        if (enemyKinds.at(i).minWave <= game.waveNumber)
        {
            eligible.push_back(static_cast<int>(i));
        }
    }
    if (eligible.empty())
    {
        return;
    }

    const int kindIndex =
        eligible.at(static_cast<size_t>(GetRandomValue(0, static_cast<int>(eligible.size()) - 1)));
    spawnEnemyAt(game, kindIndex, spawnRingPosition(game));
}

auto spawnRingPosition(const Game& game) -> Vector2
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(420, 520));
    return Vector2Add(game.player.position,
                      Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
}

void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos)
{
    if (static_cast<int>(game.enemies.size()) >= UpdateConstants::maxEnemies)
    {
        return;
    }

    const auto& kind = enemyKinds.at(static_cast<size_t>(kindIndex));
    const bool elite = GetRandomValue(0, 999) < static_cast<int32_t>(eliteChance * 1000);

    auto health = static_cast<int32_t>(
        static_cast<float>(kind.health) *
        difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyHealthMult *
        waveEnemyScale(game));
    if (elite)
    {
        health *= 2;
    }

    game.enemies.push_back(Enemy{.kind = kindIndex,
                                 .position = pos,
                                 .velocity = Vector2{},
                                 .health = health,
                                 .active = true,
                                 .stateTimer = kind.fireInterval + kind.spawnInterval + 1,
                                 .charging = false,
                                 .telegraphing = false,
                                 .phased = false,
                                 .orbitAngle = 0,
                                 .orbitDist = 200,
                                 .isElite = elite,
                                 .hitByDash = false});
}

// bossMoveCountForDifficulty is how many moves get sampled into a boss's
// moveset at spawn - "loads of moves" in the shared pool, but each fight
// only draws a few, sized by difficulty.
auto bossMoveCountForDifficulty(Difficulty difficulty) -> int
{
    switch (difficulty)
    {
    case Difficulty::Easy:
        return 2;
    case Difficulty::Hard:
        return 4;
    default:
        return 3;
    }
}

// sampleBossMoveset picks count distinct moves from the full shared
// BossAttack pool via a partial Fisher-Yates shuffle (same technique as
// sampleDistinct/rollRewardChoices) - this is the "mix and match" a boss
// gets at spawn.
auto sampleBossMoveset(int count) -> std::vector<BossAttack>
{
    std::vector<BossAttack> pool;
    pool.reserve(bossAttackCount);
    for (int i = 0; i < bossAttackCount; i++)
    {
        if (const auto attack = static_cast<BossAttack>(i); DemoConfig::isBossAttackAllowed(attack))
        {
            pool.push_back(attack);
        }
    }

    count = std::min(count, static_cast<int>(pool.size()));
    for (int i = 0; i < count; i++)
    {
        const int32_t j = i + GetRandomValue(0, static_cast<int32_t>(pool.size()) - i - 1);
        std::swap(pool.at(static_cast<size_t>(i)), pool.at(static_cast<size_t>(j)));
    }
    pool.resize(static_cast<size_t>(count));
    return pool;
}

// spawnBossInstance places a single boss, its health/size scaled by
// healthMult/sizeMult (mini vs mega) and by its randomly-picked BossType's
// own multipliers, off the shared mega-boss cadence's tier progression
// (waveNumber / megaBossWaveInterval) so mini and mega bosses keep climbing
// together as waves go on, just at different strengths. Its moveset is
// sampled fresh from the shared move pool, sized by difficulty.
void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm)
{
    const int32_t tier = game.waveNumber / megaBossWaveInterval;

    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(1200, 1500));
    const Vector2 spawnPos = Vector2Add(
        game.player.position, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

    size_t typeIndex =
        static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(bossTypes.size()) - 1));
    if constexpr (DemoConfig::isDemoBuild)
    {
        typeIndex = DemoConfig::allowedBossTypeIndices.at(static_cast<size_t>(GetRandomValue(
            0, static_cast<int32_t>(DemoConfig::allowedBossTypeIndices.size()) - 1)));
    }
    const auto& type = bossTypes.at(typeIndex);

    const auto health = static_cast<int32_t>(
        static_cast<float>(500 + tier * 250) * healthMult * type.healthMult *
        difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyHealthMult);
    const float size = 100.0F * sizeMult * type.sizeMult;

    auto moveset = sampleBossMoveset(bossMoveCountForDifficulty(game.settings.difficulty));
    const BossAttack firstAttack = moveset.front();

    game.bosses.push_back(Boss{.position = spawnPos,
                               .size = Vector2{.x = size, .y = size},
                               .color = type.color,
                               .baseColor = type.color,
                               .health = health,
                               .maxHealth = health,
                               .state = BossState::IDLE,
                               .attack = firstAttack,
                               .moveset = std::move(moveset),
                               .attackTimer = static_cast<float>(GetRandomValue(15, 35)) / 10.0F,
                               .stateTimer = 0,
                               .targetPosition = Vector2{},
                               .beamRotation = 0,
                               .slamHit = false,
                               .beamShieldLatched = false,
                               .wormholeBeamOrigin = Vector2{},
                               .chargeVelocity = Vector2{},
                               .barrageTimer = 0,
                               .hitByDash = false,
                               .isMega = isMega,
                               .isSwarm = isSwarm});
}

// spawnBossWave spawns one boss of the given tier, except every
// bossSwarmInterval-th boss spawn (mini or mega, counted together) brings 3
// at once.
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega)
{
    game.bossSpawnCount++;
    const bool isSwarm = game.bossSpawnCount % bossSwarmInterval == 0;
    const int count = isSwarm ? 3 : 1;
    for (int i = 0; i < count; i++)
    {
        spawnBossInstance(game, healthMult, sizeMult, isMega, isSwarm);
    }
    playSFX(game, game.sounds.bossWindUp);
}

void spawnBoss(Game& game) { spawnBossWave(game, megaBossHealthMult, megaBossSizeMult, true); }

void spawnMiniboss(Game& game) { spawnBossWave(game, miniBossHealthMult, miniBossSizeMult, false); }

// spawnSwarmBoss forces the every-50th-spawn 3-boss swarm on demand (sandbox
// only) without waiting for game.bossSpawnCount to naturally reach it.
void spawnSwarmBoss(Game& game)
{
    for (int i = 0; i < 3; i++)
    {
        spawnBossInstance(game, megaBossHealthMult, megaBossSizeMult, true, true);
    }
    playSFX(game, game.sounds.bossWindUp);
}

// updateBossMovement has the boss approach from wherever it spawned (usually
// far off-camera) toward the player - a bit faster than the player's own
// (possibly skill-boosted) move speed, so it can't be outrun forever - but it
// settles at bossEngageDistance rather than closing in all the way to melee
// range; if the player runs further off, it resumes closing.
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    const Vector2 toPlayer = Vector2Subtract(game.player.position, bossCenter);
    if (const float dist = Vector2Length(toPlayer); dist > bossEngageDistance)
    {
        const float chaseSpeed = game.player.speed * 1.15F + 1;
        const Vector2 drift = Vector2Scale(Vector2Normalize(toPlayer), chaseSpeed);
        boss.position = Vector2Add(boss.position, Vector2Scale(drift, deltaTime * frameScale));
    }
}

// updatePickups magnet-pulls XP gems within pickup radius toward the player
// and collects any that reach them. Pickups expire if left too long, and a
// black hole destroys (not collects) anything that drifts into it.
void updatePickups(Game& game, float deltaTime)
{
    const float magnetRadius =
        70 +
        70 * static_cast<float>(game.skillLevels.at(static_cast<size_t>(SkillType::PickupRadius))) *
            0.2F;

    for (auto& pickup : game.pickups)
    {
        if (!pickup.active)
        {
            continue;
        }

        pickup.lifetime -= deltaTime;
        if (pickup.lifetime <= 0)
        {
            pickup.active = false;
            continue;
        }

        if (game.blackhole.active &&
            Vector2Distance(pickup.position, game.blackhole.position) <= game.blackhole.radius)
        {
            pickup.active = false;
            continue;
        }

        const Vector2 toPlayer = Vector2Subtract(game.player.position, pickup.position);
        const float dist = Vector2Length(toPlayer);

        if (dist <= magnetRadius && dist > 0)
        {
            const Vector2 pull = Vector2Scale(Vector2Normalize(toPlayer), pickupMagnetSpeed);
            pickup.position =
                Vector2Add(pickup.position, Vector2Scale(pull, deltaTime * frameScale));
        }

        if (dist <= game.player.radius + 6)
        {
            pickup.active = false;

            switch (pickup.type)
            {
            case PickupType::XP:
                game.xp += pickup.value;
                break;
            case PickupType::LifeOrb:
                collectLifeOrb(game);
                break;
            case PickupType::Shield:
                if (game.player.shieldStacks < playerConstants::maxShieldStack)
                {
                    game.player.shieldStacks++;
                }
                playSFX(game, game.sounds.menuConfirm);
                break;
            default:
                break;
            }
        }
    }
}

// collectLifeOrb: two orbs make one extra heart; a single orb alone doesn't
// count toward health yet (see Player::halfLifeOrb).
void collectLifeOrb(Game& game)
{
    if (!game.player.halfLifeOrb)
    {
        game.player.halfLifeOrb = true;
        return;
    }

    game.player.halfLifeOrb = false;
    if (game.player.health < game.player.maxHealth)
    {
        game.player.health++;
    }
}

void startLevelUp(Game& game)
{
    game.xp -= game.xpToNext;
    game.level++;
    game.xpToNext = 100 + (game.level - 1) * 60;

    game.pendingChoices = rollLevelUpChoices(game);
    game.menuIndex = 0;
    game.state = GameState::LEVEL_UP;
    playSFX(game, game.sounds.menuConfirm);
}

// isFusedPassive reports whether id is the linked passive of an already
// -evolved weapon: once fused, that passive's level/effect is still exactly
// what weaponDamage/chargeRegenDuration/etc. already read from
// game.skillLevels (nothing is reset), but it no longer counts as its own
// ability slot or gets offered for further leveling - it's now embedded in
// the evolved weapon's single slot.
auto isFusedPassive(const Game& game, SkillType id) -> bool
{
    return std::ranges::any_of(
        game.weapons, [&](const Weapon& w)
        { return w.evolved && skillLinkedPassive.at(static_cast<size_t>(w.type)) == id; });
}

// equippedSlotCount is how many of the 6 ability slots are currently filled
// (any skill - weapon-grant or passive - with a level above 0, excluding
// passives already fused into an evolved weapon).
auto equippedSlotCount(const Game& game) -> int
{
    int count = 0;
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.skillLevels.at(i) > 0 && !isFusedPassive(game, id))
        {
            count++;
        }
    }
    return count;
}

auto hasWeapon(const Game& game, WeaponType kind) -> bool
{
    return std::ranges::any_of(game.weapons, [kind](const Weapon& w) { return w.type == kind; });
}

// sampleDistinct returns up to count random, non-repeating entries from ids
// (fewer if the pool is smaller) via a partial Fisher-Yates shuffle - this is
// what stops the level-up picker from ever offering the same skill twice.
auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>
{
    if (count > static_cast<int>(ids.size()))
    {
        count = static_cast<int>(ids.size());
    }
    for (int i = 0; i < count; i++)
    {
        const int32_t j = i + GetRandomValue(0, static_cast<int32_t>(ids.size()) - i - 1);
        std::swap(ids.at(static_cast<size_t>(i)), ids.at(static_cast<size_t>(j)));
    }
    ids.resize(static_cast<size_t>(count));
    return ids;
}

// demoSkillAllowed hides a weapon's grant-skill and linked-passive skill
// alike once that weapon itself isn't in the demo's allowed set - "two
// weapons and linked abilities" (see democonfig.hpp), a no-op outside demo
// builds.
auto demoSkillAllowed(SkillType id) -> bool
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        if (weaponGrantSkill.at(i) == id || skillLinkedPassive.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(static_cast<WeaponType>(i));
        }
    }
    return true;
}

// rollLevelUpChoices rolls up to 3 distinct skill picks (new skills only
// offered if there's a free ability slot; owned-but-unmaxed skills are
// always offerable) plus, if any weapon+linked-passive pair has both reached
// level 3, one guaranteed Evolve choice as a 4th option. Once literally
// nothing is left to offer (every slot full and every one of those skills
// maxed), falls back to a choice of direct Life Orb / Shield rewards instead
// of repeating an already-maxed pick.
auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>
{
    const bool slotsFull = equippedSlotCount(game) >= ItemConstants::maxAbilitySlots;

    std::vector<SkillType> eligible;
    eligible.reserve(static_cast<size_t>(SkillType::Count));
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (isFusedPassive(game, id) || game.skillLevels.at(i) >= Skills.at(i).maxLevel ||
            !demoSkillAllowed(id))
        {
            continue;
        }
        if (game.skillLevels.at(i) > 0 || !slotsFull)
        {
            eligible.push_back(id);
        }
    }

    std::vector<LevelUpChoice> choices;
    if (eligible.empty())
    {
        choices = rollRewardChoices();
        // Nothing left to level - waves keep escalating regardless (see
        // waveEnemyScale), so grant a small permanent damage bump each time
        // instead of leaving the player stuck at a fixed power level forever.
        game.postCapDamageLevels++;
    }
    else
    {
        for (const auto id : sampleDistinct(eligible, 3))
        {
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = id});
        }
    }

    std::vector<WeaponType> evolvable;
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        const auto grantSkill = weaponGrantSkill.at(i);
        const auto passive = skillLinkedPassive.at(i);
        if (hasWeapon(game, kind) && !weaponEvolved(game, kind) &&
            game.skillLevels.at(static_cast<size_t>(grantSkill)) >= 3 &&
            game.skillLevels.at(static_cast<size_t>(passive)) >= 3)
        {
            evolvable.push_back(kind);
        }
    }
    if (!evolvable.empty())
    {
        const auto pick = evolvable.at(
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(evolvable.size()) - 1)));
        choices.push_back(LevelUpChoice{.type = ChoiceType::Evolve, .weapon = pick});
    }

    return choices;
}

// rollRewardChoices offers 3 of the 6 direct Life Orb/Shield rewards
// (1/2/3 of each) - the fallback once every ability slot is full and maxed.
auto rollRewardChoices() -> std::vector<LevelUpChoice>
{
    std::vector<LevelUpChoice> pool{
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 1},
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 2},
        LevelUpChoice{.type = ChoiceType::LifeOrb, .count = 3},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 1},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 2},
        LevelUpChoice{.type = ChoiceType::Shield, .count = 3},
    };

    const auto n = static_cast<int32_t>(pool.size());
    for (int32_t i = 0; i < 3; i++)
    {
        const int32_t j = i + GetRandomValue(0, n - i - 1);
        std::swap(pool.at(static_cast<size_t>(i)), pool.at(static_cast<size_t>(j)));
    }
    pool.resize(3);
    return pool;
}

auto weaponEvolved(const Game& game, WeaponType kind) -> bool
{
    for (const auto& w : game.weapons)
    {
        if (w.type == kind)
        {
            return w.evolved;
        }
    }
    return false;
}

void applySkill(Game& game, SkillType id)
{
    game.skillLevels.at(static_cast<size_t>(id))++;

    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        grantOrLevelWeapon(game, *kind);
        return;
    }

    switch (id)
    {
    case SkillType::MoveSpeed:
        game.player.speed *= 1.1F;
        break;
    case SkillType::MaxHp:
        game.player.maxHealth++;
        game.player.health = game.player.maxHealth;
        break;
    default:
        // Damage, PickupRadius, Cooldown, Barrier are read directly from
        // game.skillLevels where needed (weaponDamage, magnetRadius,
        // weaponCooldown, updateShieldAndBarrier) - no extra state here.
        break;
    }
}

auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>
{
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        if (weaponGrantSkill.at(i) == id)
        {
            return static_cast<WeaponType>(i);
        }
    }
    return std::nullopt;
}

void grantOrLevelWeapon(Game& game, WeaponType kind)
{
    for (auto& w : game.weapons)
    {
        if (w.type == kind)
        {
            w.level++;
            return;
        }
    }
    game.weapons.push_back(Weapon{.type = kind, .level = 1});
}

// applyEvolution fuses a weapon with its linked passive into a super weapon:
// same slot, but a flat power/behavior bonus applied at fire time (see
// updateWeapons) plus a distinct look (see draw.cpp).
void applyEvolution(Game& game, WeaponType kind)
{
    for (auto& w : game.weapons)
    {
        if (w.type == kind)
        {
            w.evolved = true;
            triggerShake(game, 10, 0.3F);
            playSFX(game, game.sounds.critical);
            return;
        }
    }
}

void updateBullets(Game& game, float deltaTime)
{
    for (auto& bullet : game.bullets)
    {
        if (!bullet.active)
        {
            continue;
        }

        bullet.position =
            Vector2Add(bullet.position, Vector2Scale(bullet.velocity, deltaTime * frameScale));
        applyWormholeTransit(game, bullet.position, bullet.velocity, bullet.radius);

        for (auto& boss : game.bosses)
        {
            const Rectangle bossRect{.x = boss.position.x,
                                     .y = boss.position.y,
                                     .width = boss.size.x,
                                     .height = boss.size.y};
            if (bullet.active && boss.health > 0 &&
                CheckCollisionCircleRec(bullet.position, bullet.radius, bossRect))
            {
                bullet.active = false;
                boss.health -= bullet.damage;
                recordDamage(game, DamageSource::Forward, bullet.damage);
                // Score for a boss kill is awarded once, at the end-of-frame
                // boss-death sweep in updateGameplay - not here - so every
                // kill method (bullet, ram, AoE) pays out exactly once.
            }
        }

        for (auto& asteroid : game.asteroids)
        {
            if (bullet.active && asteroid.active &&
                CheckCollisionCircles(bullet.position, bullet.radius, asteroid.position,
                                      asteroid.radius))
            {
                bullet.active = false;
                asteroid.active = false;
                game.score += asteroidScore(asteroid.tier);
                breakAsteroid(game.asteroids, asteroid);
                playSFX(game, game.sounds.explosion);
            }
        }

        for (size_t j = 0; j < game.eliteHazards.size(); j++)
        {
            auto& hazard = game.eliteHazards.at(j);
            if (bullet.active && hazard.active &&
                CheckCollisionCircles(bullet.position, bullet.radius, hazard.position,
                                      EliteHazardConstants::radius))
            {
                bullet.active = false;
                damageEliteHazard(game, j, bullet.damage);
                recordDamage(game, DamageSource::Forward, bullet.damage);
            }
        }

        for (size_t j = 0; j < game.enemies.size(); j++)
        {
            auto& enemy = game.enemies.at(j);
            if (!bullet.active || !enemy.active || enemy.phased)
            {
                continue;
            }
            const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
            if (CheckCollisionCircles(bullet.position, bullet.radius, enemy.position, kind.radius))
            {
                bullet.active = false;
                damageEnemy(game, j, bullet.damage);
                recordDamage(game, DamageSource::Forward, bullet.damage);
            }
        }

        for (auto& projectile : game.bossProjectiles)
        {
            if (bullet.active && projectile.active && !projectile.fromPlayer &&
                CheckCollisionCircles(bullet.position, bullet.radius, projectile.position,
                                      projectile.radius))
            {
                bullet.active = false;
                projectile.health -= 1;
                if (projectile.health <= 0)
                {
                    projectile.active = false;
                    game.score += 5;
                }
            }
        }

        if (Vector2Distance(bullet.position, game.player.position) > entityDespawnRadius)
        {
            bullet.active = false;
        }
    }
}

void spawnPickup(Game& game, Vector2 position, int value, PickupType type)
{
    const float lifetime = type == PickupType::XP ? xpPickupLifetime : bonusPickupLifetime;
    game.pickups.push_back(Pickup{.position = position,
                                  .value = value,
                                  .type = type,
                                  .active = true,
                                  .lifetime = lifetime,
                                  .maxLifetime = lifetime});
}

// damageEnemy applies damage, handles death (score/XP/split/explode), and
// plays feedback - the single path all enemy damage flows through.
void damageEnemy(Game& game, size_t index, int32_t amount)
{
    const auto kind = enemyKinds.at(static_cast<size_t>(game.enemies.at(index).kind));
    game.enemies.at(index).health -= amount;

    if (game.enemies.at(index).health > 0)
    {
        return;
    }

    game.enemies.at(index).active = false;
    int32_t score = kind.score;
    if (game.enemies.at(index).isElite)
    {
        score *= 2;
    }
    game.score += score;
    playSFX(game, game.sounds.explosion);
    gainNerve(game);

    spawnPickup(game, game.enemies.at(index).position, score, PickupType::XP);

    const Vector2 bonusPos = Vector2Add(game.enemies.at(index).position, Vector2{.x = 8, .y = 8});
    const int32_t roll = GetRandomValue(0, 999);
    if (roll < shieldDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::Shield);
    }
    else if (roll < shieldDropChance + lifeOrbDropChance)
    {
        spawnPickup(game, bonusPos, 0, PickupType::LifeOrb);
    }

    if (kind.splitsOnDeath)
    {
        for (int i = 0; i < kind.splitCount; i++)
        {
            const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
            const Vector2 offset{.x = std::cos(angle) * 10, .y = std::sin(angle) * 10};
            spawnEnemyAt(game, kind.splitKind, Vector2Add(game.enemies.at(index).position, offset));
        }
    }

    if (kind.explodesOnDeath &&
        Vector2Distance(game.player.position, game.enemies.at(index).position) <=
            kind.explodeRadius + game.player.radius)
    {
        damagePlayer(game, enemyDamage(game, kind.explodeDamage));
    }
}

// killEnemyForBossAttack destroys the enemy at index without going through
// contact/bullet damage. On Normal/Hard, a boss's own attacks (Slam, Beam,
// ShockwaveStomp, ...) clearing enemies out of their blast is free for the
// boss - no score/XP/pickups - so players can't just farm boss AoE for loot;
// alwaysLoot (the boss's own death shockwave, and Easy difficulty) is the
// deliberate exception, paying out normally as a "cash-in" reward.
void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot)
{
    if (alwaysLoot || game.settings.difficulty == Difficulty::Easy)
    {
        damageEnemy(game, index, 999);
    }
    else
    {
        game.enemies.at(index).active = false;
    }
}

void updateAsteroids(Game& game, float deltaTime)
{
    for (auto& asteroid : game.asteroids)
    {
        if (!asteroid.active)
        {
            continue;
        }

        asteroid.position =
            Vector2Add(asteroid.position, Vector2Scale(asteroid.velocity, deltaTime * frameScale));

        if (Vector2Distance(asteroid.position, game.player.position) > asteroidDespawnRadius)
        {
            asteroid.active = false;
            continue;
        }

        if (game.blackhole.active)
        {
            const Vector2 toHole = Vector2Subtract(game.blackhole.position, asteroid.position);
            const float dist = Vector2Length(toHole);

            if (dist <= game.blackhole.influenceRadius && dist > 0)
            {
                const Vector2 pull = Vector2Scale(Vector2Normalize(toHole), blackHoleAsteroidPull);
                asteroid.position =
                    Vector2Add(asteroid.position, Vector2Scale(pull, deltaTime * frameScale));
            }

            if (dist <= game.blackhole.radius)
            {
                asteroid.active = false;
                breakAsteroid(game.asteroids, asteroid);
                continue;
            }
        }

        if (game.player.health > 0 && game.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.player.position, game.player.radius, asteroid.position,
                                  asteroid.radius))
        {
            if (game.player.shieldActive)
            {
                game.player.shieldActive = false;
                game.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
            }
            else
            {
                damagePlayer(game, enemyDamage(game, 1));
            }

            asteroid.active = false;
            breakAsteroid(game.asteroids, asteroid);
        }
    }
}

// updateEnemies dispatches movement/attack behavior per EnemyKind::pattern
// and resolves player contact damage.
void updateEnemies(Game& game, float deltaTime)
{
    // Spawner spawns more enemies mid-loop (spawnEnemyAt push_back's into
    // this same vector); reserving up front to the hard population cap
    // guarantees that push_back never reallocates, so the `enemy` reference
    // taken below stays valid for the rest of this iteration. Also reserved
    // once in game.cpp's resetRun, so this is idempotent defense-in-depth
    // for the (currently never hit) case where a boss AoE kill triggers a
    // splits-on-death spawn before updateEnemies has run yet this run.
    game.enemies.reserve(static_cast<size_t>(UpdateConstants::maxEnemies));

    for (size_t i = 0; i < game.enemies.size(); i++)
    {
        auto& enemy = game.enemies.at(i);
        if (!enemy.active)
        {
            continue;
        }

        const auto& kind = enemyKinds.at(static_cast<size_t>(enemy.kind));
        float speedMod = enemy.isElite ? 1.2F : 1.0F;

        // A Warlord's buff applies to every enemy on screen, not just within
        // EliteHazardConstants::auraRadius - the aura ring drawn around it
        // (see drawEliteHazard) is a visual read on which hazard is doing
        // it, not a range gate.
        for (const auto& hazard : game.eliteHazards)
        {
            if (hazard.active && hazard.role == EliteHazardRole::Warlord)
            {
                speedMod *= warlordSpeedBuff;
                break;
            }
        }

        switch (kind.pattern)
        {
        case EnemyPattern::Chase:
        {
            const Vector2 dir = Vector2Subtract(game.player.position, enemy.position);
            if (Vector2Length(dir) > 0)
            {
                enemy.position = Vector2Add(
                    enemy.position, Vector2Scale(Vector2Normalize(dir),
                                                 kind.speed * speedMod * deltaTime * frameScale));
            }
            break;
        }
        case EnemyPattern::Zigzag:
        {
            Vector2 dir = Vector2Subtract(game.player.position, enemy.position);
            if (Vector2Length(dir) > 0)
            {
                dir = Vector2Normalize(dir);
                const Vector2 perp{.x = -dir.y, .y = dir.x};
                const float wobble =
                    std::sin(static_cast<float>(GetTime()) * 5 + static_cast<float>(i)) * 1.5F;
                const Vector2 move = Vector2Add(Vector2Scale(dir, kind.speed * speedMod),
                                                Vector2Scale(perp, wobble));
                enemy.position =
                    Vector2Add(enemy.position, Vector2Scale(move, deltaTime * frameScale));
            }
            break;
        }
        case EnemyPattern::Charge:
            enemy.stateTimer -= deltaTime;
            if (enemy.telegraphing)
            {
                // Direction is already locked in below - just visibly
                // winding up (see drawEnemy's linear indicator) so there's a
                // real window to read and dodge before it actually moves.
                if (enemy.stateTimer <= 0)
                {
                    enemy.telegraphing = false;
                    enemy.charging = true;
                    enemy.stateTimer = enemyChargeDashDuration;
                }
            }
            else if (!enemy.charging)
            {
                if (enemy.stateTimer <= 0)
                {
                    enemy.telegraphing = true;
                    enemy.stateTimer = chargeTelegraphDuration;
                    const Vector2 dir =
                        Vector2Normalize(Vector2Subtract(game.player.position, enemy.position));
                    enemy.velocity = Vector2Scale(
                        dir, kind.speed * speedMod * UpdateConstants::enemyChargeDashSpeedMult);
                }
            }
            else
            {
                enemy.position = Vector2Add(enemy.position,
                                            Vector2Scale(enemy.velocity, deltaTime * frameScale));
                if (enemy.stateTimer <= 0)
                {
                    enemy.charging = false;
                    enemy.stateTimer = 1.2F;
                    enemy.velocity = Vector2{};
                }
            }
            break;
        case EnemyPattern::Orbit:
            enemy.orbitAngle += 1.5F * speedMod * deltaTime;
            if (enemy.orbitDist > kind.radius + 20)
            {
                // 9/sec, matching the frame-based rate this replaced (was
                // -0.15/frame at the assumed 60fps) so the spiral-in speed
                // stays in sync with orbitAngle's already-deltaTime-scaled
                // rotation at any frame rate.
                enemy.orbitDist -= 9 * speedMod * deltaTime;
            }
            enemy.position = Vector2Add(game.player.position,
                                        Vector2{.x = std::cos(enemy.orbitAngle) * enemy.orbitDist,
                                                .y = std::sin(enemy.orbitAngle) * enemy.orbitDist});
            break;
        case EnemyPattern::Turret:
            enemy.stateTimer -= deltaTime;
            if (enemy.stateTimer <= 0 &&
                Vector2Distance(game.player.position, enemy.position) < turretFireRange)
            {
                enemy.stateTimer = kind.fireInterval / speedMod;
                const Vector2 dir =
                    Vector2Normalize(Vector2Subtract(game.player.position, enemy.position));
                const auto turretProjectileHealth = static_cast<int32_t>(std::max(
                    1.0F, static_cast<float>(baseProjectileHealth) *
                              difficultyDefs.at(static_cast<size_t>(game.settings.difficulty))
                                  .enemyHealthMult *
                              waveEnemyScale(game)));
                game.bossProjectiles.push_back(
                    BossProjectile{.position = enemy.position,
                                   .velocity = Vector2Scale(dir, kind.projectileSpeed),
                                   .radius = 6,
                                   .homing = false,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = crossfireProjectileDamage,
                                   .health = turretProjectileHealth});
            }
            break;
        case EnemyPattern::Spawner:
            enemy.stateTimer -= deltaTime;
            if (enemy.stateTimer <= 0)
            {
                enemy.stateTimer = kind.spawnInterval / speedMod;
                for (int s = 0; s < kind.spawnCount; s++)
                {
                    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
                    const Vector2 offset{.x = std::cos(angle) * 30, .y = std::sin(angle) * 30};
                    spawnEnemyAt(game, kind.spawnKind, Vector2Add(enemy.position, offset));
                }
            }
            break;
        case EnemyPattern::Stationary:
            // doesn't move; pure contact hazard - never falls into a wormhole.
            break;
        }

        if (kind.pattern != EnemyPattern::Stationary)
        {
            applyWormholeTransit(game, enemy.position, enemy.velocity, kind.radius);
        }

        if (kind.phaseCycle)
        {
            enemy.stateTimer -= deltaTime;
            if (enemy.stateTimer <= 0)
            {
                enemy.phased = !enemy.phased;
                enemy.stateTimer = 1.5F;
            }
        }

        if (Vector2Distance(enemy.position, game.player.position) > entityDespawnRadius)
        {
            enemy.active = false;
            continue;
        }

        if (enemy.phased)
        {
            continue;
        }

        const bool collides = game.player.health > 0 &&
                              CheckCollisionCircles(game.player.position, game.player.radius,
                                                    enemy.position, kind.radius);

        // Dashing takes priority over normal contact rules: shield+dash plows
        // through and destroys everything in the path with no self-damage;
        // dashing alone still damages the enemy but costs the player health
        // per enemy hit (ignores the usual post-hit immunity window, since
        // it's meant to sting for every enemy plowed through in one dash).
        if (collides && game.player.dashing && !enemy.hitByDash)
        {
            enemy.hitByDash = true;
            if (game.player.shieldActive)
            {
                damageEnemy(game, i, 999);
                recordDamage(game, DamageSource::Dash, 999);
            }
            else
            {
                damageEnemy(game, i, dashDamage);
                recordDamage(game, DamageSource::Dash, dashDamage);
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }
            // Dash-refund-on-kill: plowing an enemy down mid-dash knocks a
            // chunk off the charge-regen timer, rewarding chaining dashes
            // through a crowd instead of one-and-done.
            if (!enemy.active)
            {
                game.player.chargeRegenTimer =
                    std::max(0.0F, game.player.chargeRegenTimer - dashKillChargeRefund);
            }
            continue;
        }

        if (collides && !game.player.dashing && game.player.immunityTimer <= 0)
        {
            if (kind.isLeech)
            {
                game.player.slowTimer = 2.0F;
                game.player.immunityTimer = 1.0F;
                damageEnemy(game, i, 999);
            }
            else if (game.player.shieldActive)
            {
                game.player.shieldActive = false;
                game.player.shieldCooldownTimer = UpdateConstants::shieldCooldownDuration;
                damageEnemy(game, i, 999);
                // Same reward as a dash-kill: blocking a hit with your
                // shield's timing and destroying the attacker on impact
                // knocks a chunk off the charge-regen timer.
                if (!enemy.active)
                {
                    game.player.chargeRegenTimer =
                        std::max(0.0F, game.player.chargeRegenTimer - shieldKillChargeRefund);
                }
            }
            else
            {
                damagePlayer(game, enemyDamage(game, kind.contactDamage));
            }
        }
    }
}

void updateProjectiles(Game& game, float deltaTime)
{
    for (auto& projectile : game.bossProjectiles)
    {
        if (!projectile.active)
        {
            continue;
        }

        if (projectile.homing)
        {
            if (projectile.fromPlayer)
            {
                // Player-fired: keep chasing the nearest enemy; if none, keep
                // flying straight (never retarget the player who fired it).
                if (const auto target = nearestEnemy(game, projectile.position); target.has_value())
                {
                    const Vector2 direction =
                        Vector2Normalize(Vector2Subtract(*target, projectile.position));
                    projectile.velocity = Vector2Scale(direction, homingProjSpeed * 1.5F);
                }
            }
            else
            {
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.player.position, projectile.position));
                projectile.velocity = Vector2Scale(direction, homingProjSpeed);
            }
        }

        projectile.position = Vector2Add(projectile.position,
                                         Vector2Scale(projectile.velocity, deltaTime * frameScale));
        applyWormholeTransit(game, projectile.position, projectile.velocity, projectile.radius);

        if (Vector2Distance(projectile.position, game.player.position) > entityDespawnRadius)
        {
            projectile.active = false;
            continue;
        }

        // Boss-fired projectiles destroy every obstacle/enemy they touch but
        // are never stopped by them - only sustained player fire (see
        // updateBullets) can shoot one down. The player's own homing
        // missiles keep the old explode-on-first-hit behavior.
        for (auto& asteroid : game.asteroids)
        {
            if (!asteroid.active || !CheckCollisionCircles(projectile.position, projectile.radius,
                                                           asteroid.position, asteroid.radius))
            {
                continue;
            }

            asteroid.active = false;
            breakAsteroid(game.asteroids, asteroid);

            if (projectile.fromPlayer)
            {
                projectile.active = false;
                break;
            }
        }

        for (size_t j = 0; j < game.enemies.size(); j++)
        {
            auto& enemy = game.enemies.at(j);
            if (!projectile.active || !enemy.active || enemy.phased ||
                !CheckCollisionCircles(projectile.position, projectile.radius, enemy.position,
                                       enemyKinds.at(static_cast<size_t>(enemy.kind)).radius))
            {
                continue;
            }

            if (projectile.fromPlayer)
            {
                projectile.active = false;
                damageEnemy(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }

            enemy.active = false; // the boss's own projectile, no player reward
        }

        if (projectile.fromPlayer)
        {
            for (size_t j = 0; j < game.eliteHazards.size(); j++)
            {
                auto& hazard = game.eliteHazards.at(j);
                if (!projectile.active || !hazard.active ||
                    !CheckCollisionCircles(projectile.position, projectile.radius, hazard.position,
                                           EliteHazardConstants::radius))
                {
                    continue;
                }

                projectile.active = false;
                damageEliteHazard(game, j, projectile.damage);
                recordDamage(game, DamageSource::Homing, projectile.damage);
                break;
            }
        }

        if (projectile.active && !projectile.fromPlayer && game.player.health > 0 &&
            !game.player.shieldActive && !game.player.dashing && game.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.player.position, game.player.radius, projectile.position,
                                  projectile.radius))
        {
            projectile.active = false;
            damagePlayer(game, enemyDamage(game, projectile.damage));
        }
    }
}

void filterDeadEntities(Game& game)
{
    std::erase_if(game.asteroids, [](const Asteroid& a) { return !a.active; });
    std::erase_if(game.bullets, [](const Bullet& b) { return !b.active; });
    std::erase_if(game.bossProjectiles, [](const BossProjectile& p) { return !p.active; });
    std::erase_if(game.enemies, [](const Enemy& e) { return !e.active; });
    std::erase_if(game.eliteHazards, [](const EliteHazard& h) { return !h.active; });
    std::erase_if(game.pickups, [](const Pickup& p) { return !p.active; });
    std::erase_if(game.mines, [](const Mine& m) { return !m.active; });
}

// updateBgParticles drifts the decorative background motes. Positions are
// tile-space offsets (see tiledWorldPos in draw.cpp), not world positions.
void updateBgParticles(Game& game)
{
    const auto tileW = static_cast<float>(game.screenWidth);
    const auto tileH = static_cast<float>(game.screenHeight);

    for (auto& p : game.bgParticles)
    {
        p.position = Vector2Add(p.position, p.velocity);

        if (p.position.x < 0)
        {
            p.position.x += tileW;
        }
        if (p.position.x > tileW)
        {
            p.position.x -= tileW;
        }
        if (p.position.y < 0)
        {
            p.position.y += tileH;
        }
        if (p.position.y > tileH)
        {
            p.position.y -= tileH;
        }
    }
}

void damageEliteHazard(Game& game, size_t index, int32_t amount)
{
    auto& hazard = game.eliteHazards.at(index);
    hazard.health -= amount;
    if (hazard.health > 0)
    {
        return;
    }

    hazard.active = false;
    game.score += eliteHazardScore;
    playSFX(game, game.sounds.explosion);
    gainNerve(game);

    // A deliberate risk/reward kill, not a routine one - guaranteed reward.
    spawnPickup(game, hazard.position, 0,
                GetRandomValue(0, 1) == 0 ? PickupType::Shield : PickupType::LifeOrb);
    spawnPickup(game, hazard.position, eliteHazardXpBonus, PickupType::XP);
}

// updateEliteHazards spawns/despawns hazards on a long timer (capped
// concurrently by difficulty) and drifts each one toward a point near the
// edge of the player's screen, slowly orbiting - it holds position rather
// than chasing, and applies its buff/debuff aura for as long as it's alive
// (see weaponCooldown for Suppressor, updateEnemies for Warlord).
// spawnEliteHazard places one hazard of the given role out on the
// screen-edge orbit ring (not on top of the player, so it doesn't land an
// unavoidable free hit the instant it appears - see updateEliteHazards'
// follow loop). Shared by the natural auto-spawn path and the sandbox's
// on-command hotkey (see updateSandboxInput); the sandbox path ignores
// eliteHazardCap by design (spawn whatever's asked for, for testing).
void spawnEliteHazard(Game& game, EliteHazardRole role)
{
    // The visible world viewport is ~screenWidth x screenHeight world units -
    // the pixelScale render-target downscale and the 1/pixelScale camera
    // zoom cancel out (see beginWorldCamera) - so half-extents are just
    // screenWidth/2 x screenHeight/2, NOT multiplied by pixelScale again
    // (that placed hazards ~3x past the actual screen edge, off-camera).
    const float halfExtentX = static_cast<float>(game.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    const auto health = static_cast<int32_t>(
        static_cast<float>(eliteHazardBaseHealth) *
        difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyHealthMult *
        waveEnemyScale(game));

    const auto spawnAngle = static_cast<float>(GetRandomValue(0, 359));
    const float spawnRad = spawnAngle * DEG2RAD;
    const Vector2 spawnPos =
        Vector2Add(game.player.position, Vector2{.x = std::cos(spawnRad) * orbitDist,
                                                 .y = std::sin(spawnRad) * orbitDist});

    game.eliteHazards.push_back(EliteHazard{.position = spawnPos,
                                            .angle = spawnAngle,
                                            .role = role,
                                            .health = health,
                                            .maxHealth = health,
                                            .active = true});
}

void updateEliteHazards(Game& game, float deltaTime)
{
    // The visible world viewport is ~screenWidth x screenHeight world units -
    // the pixelScale render-target downscale and the 1/pixelScale camera
    // zoom cancel out (see beginWorldCamera) - so half-extents are just
    // screenWidth/2 x screenHeight/2, NOT multiplied by pixelScale again
    // (that placed hazards ~3x past the actual screen edge, off-camera).
    const float halfExtentX = static_cast<float>(game.screenWidth) / 2;
    const float halfExtentY = static_cast<float>(game.screenHeight) / 2;
    const float orbitDist = std::min(halfExtentX, halfExtentY) * eliteHazardOrbitDistFrac;

    game.eliteHazardSpawnTimer -= deltaTime;
    // Sandbox has no auto-spawns (see enterSandbox) - hazards only ever
    // appear there via the sandbox's own spawn command.
    if (!game.sandbox && game.eliteHazardSpawnTimer <= 0)
    {
        if (static_cast<int>(game.eliteHazards.size()) < eliteHazardCap(game))
        {
            spawnEliteHazard(game, GetRandomValue(0, 1) == 0 ? EliteHazardRole::Warlord
                                                             : EliteHazardRole::Suppressor);
        }
        game.eliteHazardSpawnTimer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(eliteHazardSpawnIntervalMin),
                                              static_cast<int32_t>(eliteHazardSpawnIntervalMax)));
    }

    for (auto& hazard : game.eliteHazards)
    {
        if (!hazard.active)
        {
            continue;
        }

        hazard.angle += eliteHazardOrbitSpin * deltaTime;
        const float rad = hazard.angle * DEG2RAD;
        const Vector2 targetPos =
            Vector2Add(game.player.position,
                       Vector2{.x = std::cos(rad) * orbitDist, .y = std::sin(rad) * orbitDist});

        const float lerpT = std::clamp(eliteHazardFollowRate * deltaTime, 0.0F, 1.0F);
        hazard.position = Vector2Lerp(hazard.position, targetPos, lerpT);

        if (game.player.health > 0 && game.player.immunityTimer <= 0 &&
            CheckCollisionCircles(game.player.position, game.player.radius, hazard.position,
                                  EliteHazardConstants::radius))
        {
            damagePlayer(game, enemyDamage(game, eliteHazardContactDamage));
        }
    }
}

// spawnBlackHole activates the black hole at a random point near the player
// - the natural auto-spawn path (updateBlackHole) and the sandbox's
// on-command hotkey (see updateSandboxInput) both funnel through this.
void spawnBlackHole(Game& game)
{
    const float angle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto dist = static_cast<float>(GetRandomValue(150, 350));
    game.blackhole.position = Vector2Add(
        game.player.position, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});
    game.blackhole.radius = 25;
    game.blackhole.influenceRadius = 140;
    game.blackhole.active = true;
    game.blackhole.timer = static_cast<float>(GetRandomValue(60, 100)) / 10.0F;
}

void updateBlackHole(Game& game, float deltaTime)
{
    game.blackhole.timer -= deltaTime;

    // Sandbox has no auto-spawns (see enterSandbox) - the black hole only
    // ever appears there via the sandbox's own spawn command.
    if (!game.sandbox && !game.blackhole.active && game.blackhole.timer <= 0)
    {
        spawnBlackHole(game);
    }
    else if (game.blackhole.active && game.blackhole.timer <= 0)
    {
        game.blackhole.active = false;
        game.blackhole.timer = static_cast<float>(GetRandomValue(50, 90)) / 10.0F;
    }

    if (game.blackhole.active)
    {
        const Vector2 toPlayer = Vector2Subtract(game.player.position, game.blackhole.position);
        const float dist = Vector2Length(toPlayer);

        if (dist > game.blackhole.influenceRadius)
        {
            const Vector2 drift = Vector2Scale(Vector2Normalize(toPlayer), blackHoleChaseSpeed);
            game.blackhole.position =
                Vector2Add(game.blackhole.position, Vector2Scale(drift, deltaTime * frameScale));
        }
    }
}

// updateWormhole spawns/despawns a linked pair of portal mouths on a timer,
// the same way updateBlackHole does. Each mouth faces one of the 4 cardinal
// directions, rolled independently.
// spawnWormholePair activates a linked pair of portal mouths near the player
// - shared by the natural auto-spawn path (updateWormhole) and the sandbox's
// on-command hotkey (see updateSandboxInput).
void spawnWormholePair(Game& game)
{
    const float angleA = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distA = static_cast<float>(GetRandomValue(300, 600));
    const float angleB = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
    const auto distB = static_cast<float>(GetRandomValue(300, 600));

    game.wormhole.positionA =
        Vector2Add(game.player.position,
                   Vector2{.x = std::cos(angleA) * distA, .y = std::sin(angleA) * distA});
    game.wormhole.positionB =
        Vector2Add(game.player.position,
                   Vector2{.x = std::cos(angleB) * distB, .y = std::sin(angleB) * distB});
    game.wormhole.facingA = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.wormhole.facingB = static_cast<WormholeFacing>(GetRandomValue(0, 3));
    game.wormhole.radius = wormholeRadius;
    game.wormhole.active = true;
    game.wormhole.timer = wormholeLifetime;
}

void updateWormhole(Game& game, float deltaTime)
{
    game.wormhole.timer -= deltaTime;

    // Sandbox has no auto-spawns (see enterSandbox) - the wormhole only ever
    // appears there via the sandbox's own spawn command.
    if (!game.sandbox && !game.wormhole.active && game.wormhole.timer <= 0)
    {
        spawnWormholePair(game);
    }
    else if (game.wormhole.active && game.wormhole.timer <= 0)
    {
        game.wormhole.active = false;
        game.wormhole.timer =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeSpawnCooldownMin),
                                              static_cast<int32_t>(wormholeSpawnCooldownMax)));
    }
}

// applyWormholeTransit teleports anything that touches either mouth to the
// other: position moves to the far mouth (offset outward along its facing so
// it doesn't immediately re-trigger), and velocity is rotated by the facing
// difference between the two mouths - a real portal, not a blink. Returns
// true if a transit happened.
auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool
{
    if (!game.wormhole.active)
    {
        return false;
    }

    const bool atA = Vector2Distance(position, game.wormhole.positionA) <= game.wormhole.radius;
    const bool atB =
        !atA && Vector2Distance(position, game.wormhole.positionB) <= game.wormhole.radius;
    if (!atA && !atB)
    {
        return false;
    }

    const WormholeFacing entryFacing = atA ? game.wormhole.facingA : game.wormhole.facingB;
    const WormholeFacing exitFacing = atA ? game.wormhole.facingB : game.wormhole.facingA;
    const Vector2 exitPoint = atA ? game.wormhole.positionB : game.wormhole.positionA;

    const float deltaDegrees =
        (static_cast<float>(exitFacing) - static_cast<float>(entryFacing)) * 90.0F;
    velocity = Vector2Rotate(velocity, deltaDegrees * DEG2RAD);

    const Vector2 exitDir = wormholeFacingVector(exitFacing);

    // A mouth is a plain proximity trigger, not an oriented surface - a
    // bullet (or anything else) can wander in from any angle, not just
    // "through" the mouth along its facing. Blindly rotating by the facing
    // difference doesn't guarantee the result points away from the exit: if
    // the entry velocity happened to be angled opposite the entry facing,
    // the rotated exit velocity ends up angled opposite the exit facing too
    // - i.e. back toward the mouth it just exited - so it drifts straight
    // back in and re-triggers every frame (bullets getting "stuck" in a
    // wormhole). Reflect the inward component so the exit velocity always
    // has a component carrying it away from the mouth, regardless of the
    // angle it entered at.
    if (const float outward = Vector2DotProduct(velocity, exitDir); outward < 0)
    {
        velocity = Vector2Subtract(velocity, Vector2Scale(exitDir, 2 * outward));
    }

    position =
        Vector2Add(exitPoint, Vector2Scale(exitDir, game.wormhole.radius + entityRadius + 4));

    return true;
}

void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter)
{
    // Enraged in its last stretch of health: cooldowns and telegraphs speed up.
    const bool enraged =
        boss.health > 0 &&
        static_cast<float>(boss.health) <= static_cast<float>(boss.maxHealth) * enrageHealthFrac;
    const float rateMult = enraged ? enrageSpeedMult : 1.0F;

    switch (boss.state)
    {
    case BossState::IDLE:
        boss.attackTimer -= deltaTime;

        if (boss.attackTimer <= 0 && boss.health > 0)
        {
            boss.attack = boss.moveset.at(static_cast<size_t>(
                GetRandomValue(0, static_cast<int32_t>(boss.moveset.size()) - 1)));
            boss.state = BossState::WINDING_UP;
            boss.stateTimer = bossWindupDuration(boss.attack) * rateMult;
            boss.beamShieldLatched = false;

            switch (boss.attack)
            {
            case BossAttack::Spread:
                boss.color = Palette::BossSpread;
                game.spreadWindupShots = 0;
                break;
            case BossAttack::Slam:
                boss.color = Palette::Accent;
                break;
            case BossAttack::Beam:
                boss.color = Palette::BossBeam;
                break;
            case BossAttack::WormholeBeam:
                boss.color = Palette::Shield;
                break;
            case BossAttack::MineDrop:
                boss.color = Palette::AccentDim;
                break;
            case BossAttack::ChargeDash:
                boss.color = Palette::Crit;
                break;
            case BossAttack::SummonAdds:
                boss.color = Palette::Charge;
                break;
            case BossAttack::ShockwaveStomp:
                boss.color = Palette::Haze;
                break;
            case BossAttack::Barrage:
                boss.color = Palette::BossSpread;
                break;
            case BossAttack::GravityWell:
                boss.color = Palette::StructMid;
                break;
            case BossAttack::HomingBarrage:
                boss.color = Palette::BossHoming;
                break;
            }

            playSFX(game, game.sounds.bossWindUp);
        }
        break;
    case BossState::WINDING_UP:
        boss.stateTimer -= deltaTime;
        boss.targetPosition = game.player.position;

        if (boss.stateTimer <= 0)
        {
            startBossAttack(game, boss, bossCenter);
        }
        break;
    case BossState::SHOOTING:
        boss.stateTimer -= deltaTime;

        if (boss.attack == BossAttack::Slam)
        {
            const float progress =
                std::clamp(1 - boss.stateTimer / UpdateConstants::slamDuration, 0.0F, 1.0F);
            const float radius = bossConstants::maxSlamRadius * progress;

            for (auto& asteroid : game.asteroids)
            {
                if (asteroid.active &&
                    Vector2Distance(bossCenter, asteroid.position) <= radius + asteroid.radius)
                {
                    asteroid.active = false;
                    breakAsteroid(game.asteroids, asteroid);
                }
            }

            for (size_t i = 0; i < game.enemies.size(); i++)
            {
                const auto& e = game.enemies.at(i);
                if (e.active && !e.phased &&
                    Vector2Distance(bossCenter, e.position) <=
                        radius + enemyKinds.at(static_cast<size_t>(e.kind)).radius)
                {
                    killEnemyForBossAttack(game, i, false);
                }
            }

            if (!boss.slamHit)
            {
                if (const auto result = resolveExpandingWaveHit(game, bossCenter, radius, true);
                    result.resolved)
                {
                    boss.slamHit = true;
                    if (result.hit)
                    {
                        damagePlayer(game, enemyDamage(game, 1));
                    }
                }
            }
        }

        if (boss.attack == BossAttack::Spread)
        {
            boss.barrageTimer -= deltaTime;
            const int targetRounds =
                spreadRoundsByDifficulty.at(static_cast<size_t>(game.settings.difficulty));
            if (boss.barrageTimer <= 0 && game.spreadWindupShots < targetRounds)
            {
                boss.barrageTimer = spreadRoundInterval;
                game.spreadWindupShots++;
                playSFX(game, game.sounds.spreadBurst);
                triggerShake(game, 5, 0.2F);

                // Re-aimed at the player's current position each round, so
                // the wall of bullets tracks you round to round even though
                // no single bullet in it homes.
                const Vector2 aimDir =
                    Vector2Normalize(Vector2Subtract(game.player.position, bossCenter));
                const float baseAngle = std::atan2(aimDir.y, aimDir.x);
                const auto roundProjectileHealth = static_cast<int32_t>(std::max(
                    1.0F, static_cast<float>(baseProjectileHealth) *
                              difficultyDefs.at(static_cast<size_t>(game.settings.difficulty))
                                  .enemyHealthMult *
                              waveEnemyScale(game)));
                constexpr int32_t half = spreadBulletsPerRound / 2;
                for (int32_t i = -half; i < spreadBulletsPerRound - half; i++)
                {
                    const float angle = baseAngle + static_cast<float>(i) * 15 * DEG2RAD;
                    const Vector2 vel{.x = std::cos(angle) * spreadProjSpeed,
                                      .y = std::sin(angle) * spreadProjSpeed};

                    game.bossProjectiles.push_back(
                        BossProjectile{.position = bossCenter,
                                       .velocity = vel,
                                       .radius = 7,
                                       .homing = false,
                                       .active = true,
                                       .fromPlayer = false,
                                       .damage = crossfireProjectileDamage,
                                       .health = roundProjectileHealth});
                }
            }
        }

        if (boss.attack == BossAttack::Beam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, bossCenter));
            const Vector2 beamEnd = Vector2Add(bossCenter, Vector2Scale(direction, 2000));
            processBeamAttack(game, boss, bossCenter, beamEnd);
        }

        if (boss.attack == BossAttack::WormholeBeam)
        {
            const Vector2 direction =
                Vector2Normalize(Vector2Subtract(boss.targetPosition, boss.wormholeBeamOrigin));
            const Vector2 beamEnd =
                Vector2Add(boss.wormholeBeamOrigin, Vector2Scale(direction, 2000));
            processBeamAttack(game, boss, boss.wormholeBeamOrigin, beamEnd);
        }

        if (boss.attack == BossAttack::ChargeDash)
        {
            boss.position = Vector2Add(boss.position, Vector2Scale(boss.chargeVelocity, deltaTime));
        }

        if (boss.attack == BossAttack::Barrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = barrageFireInterval;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.player.position, bossCenter));
                // Straight-line, not homing: aimed fresh at the player's
                // current position each shot, at a speed above the player's
                // own move speed so a direct retreat can still be run down -
                // but since it's not homing, moving off that line dodges it.
                // The boss itself keeps closing distance the whole time
                // (updateBossMovement runs regardless of attack state), so
                // it stays in range to keep firing rather than falling behind.
                game.bossProjectiles.push_back(
                    BossProjectile{.position = bossCenter,
                                   .velocity = Vector2Scale(direction, barrageProjSpeed),
                                   .radius = 6,
                                   .homing = false,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = crossfireProjectileDamage,
                                   .health = 1});
                playSFX(game, game.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::HomingBarrage)
        {
            boss.barrageTimer -= deltaTime;
            if (boss.barrageTimer <= 0)
            {
                boss.barrageTimer = homingBarrageFireInterval;
                const Vector2 direction =
                    Vector2Normalize(Vector2Subtract(game.player.position, bossCenter));
                game.bossProjectiles.push_back(
                    BossProjectile{.position = bossCenter,
                                   .velocity = Vector2Scale(direction, homingProjSpeed),
                                   .radius = 6,
                                   .homing = true,
                                   .active = true,
                                   .fromPlayer = false,
                                   .damage = crossfireProjectileDamage,
                                   .health = 1});
                playSFX(game, game.sounds.spreadBurst);
            }
        }

        if (boss.attack == BossAttack::GravityWell)
        {
            const Vector2 toBoss = Vector2Subtract(bossCenter, game.player.position);
            if (Vector2Length(toBoss) > 0)
            {
                const Vector2 pull =
                    Vector2Scale(Vector2Normalize(toBoss), gravityWellPullStrength);
                game.player.position =
                    Vector2Add(game.player.position, Vector2Scale(pull, deltaTime * frameScale));
            }
        }

        if (boss.stateTimer <= 0)
        {
            boss.state = BossState::IDLE;
            boss.attackTimer = static_cast<float>(GetRandomValue(15, 40)) / 10.0F * rateMult;
            boss.color = boss.baseColor;
        }
        break;
    }
}

auto bossWindupDuration(BossAttack attack) -> float
{
    switch (attack)
    {
    case BossAttack::Slam:
        return 1.3F;
    case BossAttack::WormholeBeam:
        return 1.4F;
    case BossAttack::ChargeDash:
        return 1.2F;
    default:
        return 1.0F;
    }
}

// processBeamAttack is shared by Beam and WormholeBeam: it always destroys
// every asteroid/enemy the line touches (no line-of-sight blocking - a boss
// beam clears whatever's in its way) and damages the player if they're in
// it. A player who shields the beam and lets the shield expire while still
// standing in it eats a single heavy hit instead of the usual chip damage -
// shielding is a delay to get clear, not a way to facetank the whole attack.
void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd)
{
    for (auto& asteroid : game.asteroids)
    {
        if (asteroid.active &&
            CheckCollisionCircleLine(asteroid.position, asteroid.radius, beamStart, beamEnd))
        {
            asteroid.active = false;
            breakAsteroid(game.asteroids, asteroid);
        }
    }

    for (size_t i = 0; i < game.enemies.size(); i++)
    {
        const auto& e = game.enemies.at(i);
        if (e.active && !e.phased &&
            CheckCollisionCircleLine(e.position, enemyKinds.at(static_cast<size_t>(e.kind)).radius,
                                     beamStart, beamEnd))
        {
            killEnemyForBossAttack(game, i, false);
        }
    }

    if (game.player.health <= 0 || game.player.dashing)
    {
        return;
    }

    if (!CheckCollisionCircleLine(game.player.position, game.player.radius, beamStart, beamEnd))
    {
        return;
    }

    if (game.player.shieldActive)
    {
        boss.beamShieldLatched = true;
        return;
    }

    if (boss.beamShieldLatched)
    {
        boss.beamShieldLatched = false;
        damagePlayer(game, game.player.maxHealth);
        return;
    }

    if (game.player.immunityTimer <= 0)
    {
        damagePlayer(game, enemyDamage(game, 1));
    }
}

// forceBossAttack is the sandbox command-center hook: skips the normal
// random-pick-from-moveset roll and puts the boss straight into its windup
// for a specific attack, so every move can be tested on demand.
void forceBossAttack(Game& game, Boss& boss, BossAttack attack)
{
    boss.attack = attack;
    boss.state = BossState::WINDING_UP;
    boss.stateTimer = bossWindupDuration(attack);
    boss.beamShieldLatched = false;

    switch (attack)
    {
    case BossAttack::Spread:
        boss.color = Palette::BossSpread;
        game.spreadWindupShots = 0;
        break;
    case BossAttack::Slam:
        boss.color = Palette::Accent;
        break;
    case BossAttack::Beam:
        boss.color = Palette::BossBeam;
        break;
    case BossAttack::WormholeBeam:
        boss.color = Palette::Shield;
        break;
    case BossAttack::MineDrop:
        boss.color = Palette::AccentDim;
        break;
    case BossAttack::ChargeDash:
        boss.color = Palette::Crit;
        break;
    case BossAttack::SummonAdds:
        boss.color = Palette::Charge;
        break;
    case BossAttack::ShockwaveStomp:
        boss.color = Palette::Haze;
        break;
    case BossAttack::Barrage:
        boss.color = Palette::BossSpread;
        break;
    case BossAttack::GravityWell:
        boss.color = Palette::StructMid;
        break;
    case BossAttack::HomingBarrage:
        boss.color = Palette::BossHoming;
        break;
    }

    playSFX(game, game.sounds.bossWindUp);
}

void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter)
{
    const Vector2 direction = Vector2Subtract(boss.targetPosition, bossCenter);
    boss.beamRotation = std::atan2(direction.y, direction.x) * RAD2DEG;
    const Vector2 aimDirection = Vector2Normalize(direction);

    boss.state = BossState::SHOOTING;

    const auto projectileHealth = static_cast<int32_t>(std::max(
        1.0F, static_cast<float>(baseProjectileHealth) *
                  difficultyDefs.at(static_cast<size_t>(game.settings.difficulty)).enemyHealthMult *
                  waveEnemyScale(game)));

    switch (boss.attack)
    {
    case BossAttack::Beam:
        boss.stateTimer = beamAttackDuration;
        playSFX(game, game.sounds.beamFire);
        triggerShake(game, 5, 0.2F);
        break;
    case BossAttack::Spread:
    {
        const int rounds =
            spreadRoundsByDifficulty.at(static_cast<size_t>(game.settings.difficulty));
        boss.stateTimer = static_cast<float>(rounds) * spreadRoundInterval + 0.2F;
        boss.barrageTimer = 0;
        game.spreadWindupShots = 0; // rounds fired so far this attack
        break;
    }
    case BossAttack::Slam:
        boss.stateTimer = UpdateConstants::slamDuration;
        boss.slamHit = false;
        playSFX(game, game.sounds.slamBoom);
        triggerShake(game, 10, 0.4F);
        break;
    case BossAttack::WormholeBeam:
    {
        boss.stateTimer = wormholeBeamDuration;
        boss.beamShieldLatched = false;
        const float flankAngle = static_cast<float>(GetRandomValue(0, 359)) * DEG2RAD;
        const auto flankDist =
            static_cast<float>(GetRandomValue(static_cast<int32_t>(wormholeBeamFlankMinDist),
                                              static_cast<int32_t>(wormholeBeamFlankMaxDist)));
        boss.wormholeBeamOrigin =
            Vector2Add(game.player.position, Vector2{.x = std::cos(flankAngle) * flankDist,
                                                     .y = std::sin(flankAngle) * flankDist});
        playSFX(game, game.sounds.beamFire);
        triggerShake(game, 6, 0.25F);
        break;
    }
    case BossAttack::MineDrop:
        boss.stateTimer = 0.5F;
        playSFX(game, game.sounds.slamBoom);
        for (int i = 0; i < mineDropCount; i++)
        {
            const float angle = static_cast<float>(i) * (360.0F / mineDropCount) * DEG2RAD;
            const auto dist = static_cast<float>(GetRandomValue(60, 140));
            const Vector2 pos = Vector2Add(
                bossCenter, Vector2{.x = std::cos(angle) * dist, .y = std::sin(angle) * dist});

            game.bossProjectiles.push_back(BossProjectile{.position = pos,
                                                          .velocity = Vector2{},
                                                          .radius = mineDropRadius,
                                                          .homing = false,
                                                          .active = true,
                                                          .fromPlayer = false,
                                                          .damage = mineDropDamage,
                                                          .health = projectileHealth});
        }
        break;
    case BossAttack::ChargeDash:
        boss.stateTimer = chargeDashDuration;
        boss.chargeVelocity = Vector2Scale(aimDirection, chargeDashSpeed);
        playSFX(game, game.sounds.bossWindUp);
        triggerShake(game, 8, 0.3F);
        break;
    case BossAttack::SummonAdds:
    {
        boss.stateTimer = 0.6F;
        playSFX(game, game.sounds.spreadBurst);
        const int addsCount =
            summonAddsCountByDifficulty.at(static_cast<size_t>(game.settings.difficulty));
        for (int i = 0; i < addsCount; i++)
        {
            spawnEnemy(game);
        }
        break;
    }
    case BossAttack::ShockwaveStomp:
        boss.stateTimer = UpdateConstants::shockwaveStompDuration;
        playSFX(game, game.sounds.slamBoom);
        triggerShake(game, 10, 0.35F);
        for (auto& asteroid : game.asteroids)
        {
            if (asteroid.active && Vector2Distance(bossCenter, asteroid.position) <=
                                       UpdateConstants::shockwaveStompRadius + asteroid.radius)
            {
                asteroid.active = false;
                breakAsteroid(game.asteroids, asteroid);
            }
        }
        for (size_t i = 0; i < game.enemies.size(); i++)
        {
            const auto& e = game.enemies.at(i);
            if (e.active && !e.phased &&
                Vector2Distance(bossCenter, e.position) <=
                    UpdateConstants::shockwaveStompRadius +
                        enemyKinds.at(static_cast<size_t>(e.kind)).radius)
            {
                killEnemyForBossAttack(game, i, false);
            }
        }
        if (game.player.health > 0 && !game.player.shieldActive && !game.player.dashing &&
            Vector2Distance(bossCenter, game.player.position) <=
                UpdateConstants::shockwaveStompRadius + game.player.radius)
        {
            damagePlayer(game, enemyDamage(game, 2));
        }
        break;
    case BossAttack::Barrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.sounds.spreadBurst);
        break;
    case BossAttack::GravityWell:
        boss.stateTimer = gravityWellDuration;
        playSFX(game, game.sounds.bossWindUp);
        break;
    case BossAttack::HomingBarrage:
        boss.stateTimer = barrageDuration;
        boss.barrageTimer = 0;
        playSFX(game, game.sounds.homingLaunch);
        break;
    }
}
auto UpdateGame(Game& game, float deltaTime) -> bool
{
    if (IsKeyPressed(KEY_F11))
    {
        toggleFullscreen(game);
    }
    syncScreenSize(game);

    updateBgParticles(game);
    updateDeathParticles(game, deltaTime);
    updateBgmLayers(game, deltaTime);
    UpdateMusicStream(game.bgm.drone);
    UpdateMusicStream(game.bgm.intensity);
    UpdateMusicStream(game.bgm.upgrade);

    if (game.shakeTimer > 0)
    {
        game.shakeTimer -= deltaTime;
        if (game.shakeTimer <= 0)
        {
            game.shakeTimer = 0;
            game.shakeIntensity = 0;
        }
    }

    if (game.hitPauseTimer > 0)
    {
        game.hitPauseTimer -= deltaTime;
        return false;
    }

    switch (game.state)
    {
    case GameState::TITLE:
        return updateTitle(game);
    case GameState::GAMEPLAY:
        updateGameplay(game, deltaTime);
        break;
    case GameState::PAUSED:
        return updatePaused(game);
    case GameState::LEVEL_UP:
        return updateLevelUp(game);
    case GameState::GAME_OVER:
        return updateGameOver(game);
    case GameState::SETTINGS:
        return updateSettings(game);
    }

    return false;
}
