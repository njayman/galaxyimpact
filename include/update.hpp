#pragma once

#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "game.hpp"
#include "raylib.h"
#include <cmath>
#include <optional>
#include <utility>
#include <vector>

namespace UpdateConstants
{
constexpr float nerveMax = 100;
constexpr float chargeRegenTime = 3.0;
constexpr int32_t maxCharges = 2;
constexpr float slamDuration = 1.6F;
constexpr float bossDeathShockwaveDuration = 3.0F;
constexpr float shockFlashDuration = 0.4F;
constexpr float waveEnemyScalePerWave = 0.035F;

constexpr float playerConstantPerLevel = 0.02F;

constexpr float waveEnemyScalePerLevel = 0.03F;
constexpr float pickupExpiryWarning = 2.5F;
constexpr float shockwaveStompRadius = 220;
constexpr float shockwaveStompDuration = 0.5F;
constexpr float shieldCooldownDuration = 2.0F;
constexpr int maxEnemies = 200;
constexpr float enemyChargeDashDuration = 0.4F;
constexpr float enemyChargeDashSpeedMult = 14.0F;
constexpr float enemyChargeOvershoot = 160.0F; // charges this far past where the player was
constexpr float frameScale = 60.0F;
constexpr float hitFlashDuration = 0.12F;
constexpr float nerveBurstWindup = 0.35F;
constexpr float beltbreakerPlateHealthPerTier = 220.0F;
// Boss QTE cutscene (qte.cpp)
constexpr float qteSlowMult = 0.04F;       // world time scale while a QTE cutscene runs
constexpr float qteActionDuration = 0.18F; // ship's reaction animation
constexpr float qteDodgeDistance = 150.0F;
constexpr float qteDashDistance = 260.0F;
constexpr float qteParryLunge = 90.0F;
constexpr float qteNerveGain = 10.0F; // nerve per answered prompt
constexpr int32_t qteParryChance = 15;
constexpr int32_t qteDashChance = 15;
constexpr int32_t qteShieldChance = 15;
constexpr float qteMinGap = 20.0F; // shared rule: at least this long between Boss QTEs
constexpr float qteDamageMult = 3.0F; // shared rule: every hit on an enemy or boss during a QTE
constexpr float qteFinisherTurn = 0.22F;
constexpr float qteFinisherCharge = 0.5F;
constexpr float qteFinisherShot = 0.12F;
// A clean Boss QTE chain ends in the finisher shot; each answer charges it by this share of the
// boss's max health (then tripled by qteDamageMult): that shot is the QTE's big payoff.
constexpr float qteFinisherChargeAnswer = 0.02F;
constexpr float qteFinisherChargeParry = 0.05F;
constexpr float qteFinisherChargeNerve = 0.10F;
constexpr float qteFinisherChargeMash = 0.03F;
constexpr float qteStunWindow = 3.0F;      // generic boss after a clean Rush: no moves, +50% damage
constexpr float qteStunDamageMult = 1.5F;
// Beltbreaker plate rush
constexpr float beltbreakerPlateRushCueDuration = 0.6F;
constexpr float beltbreakerPlateRushOffscreenMargin = 140.0F;
constexpr float beltbreakerPlateRushOffscreenSpeed = 1800.0F;
constexpr float beltbreakerPlateRushPromptDuration = 0.9F; // plate arrives exactly at timeout
constexpr float beltbreakerPlateRushHitRadius = 44.0F;
constexpr float beltbreakerRushBeat = 0.35F; // pause between attacks
constexpr float beltbreakerRushLooseLingerTime = 0.7F; // missed plate's live dash before returning
constexpr int32_t beltbreakerRushAttacks = 8;
constexpr float beltbreakerRushMinAngleChange = 70.0F; // each attack comes from a new side
constexpr float beltbreakerRushShieldDamageFrac = 0.05F;
constexpr float beltbreakerRushParryDamageFrac = 0.15F;
constexpr float beltbreakerRushNerveDamageFrac = 0.35F;
constexpr float beltbreakerRushStagger = 1.0F;
constexpr float beltbreakerPunishWindow = 3.0F;     // clean clear: exposed core
constexpr float beltbreakerPunishDamageMult = 1.5F; // stacks with the 2.5x dash bonus
// Wreckworm constrict
constexpr float wreckwormConstrictCatchBand = 48.0F; // ring this close outside the player catches them
constexpr float wreckwormConstrictHoldMargin = 0.05F; // keeps the coil shut while the cutscene runs
constexpr float wreckwormConstrictMashDuration = 1.6F;
constexpr int32_t wreckwormConstrictMashPresses = 8; // 5 a second, leaving room to react
constexpr float wreckwormConstrictMashBeat = 0.6F; // lets a mash settle before the first strike
constexpr int32_t wreckwormConstrictStrikes = 8;
constexpr float wreckwormConstrictStrikeWindow = 0.8F; // segment arrives exactly at timeout
constexpr float wreckwormConstrictBeat = 0.35F;
constexpr int32_t wreckwormConstrictParryChance = 20;
constexpr float wreckwormConstrictLungeRear = 120.0F; // a striking segment rears out this far first
constexpr float wreckwormConstrictLungeRearFrac = 0.4F;
constexpr int32_t wreckwormConstrictCrushDamage = 2; // as heavy as a plate dash or ring crush
constexpr float wreckwormPunishWindow = 3.0F; // clean clear: stunned head
constexpr float wreckwormPunishDamageMult = 1.5F;
// Slagmaw Swallowed: Devour drags the player into the maw
constexpr float slagmawSwallowMashDuration = 1.6F;
constexpr int32_t slagmawSwallowMashPresses = 8;
constexpr float slagmawSwallowMashBeat = 0.5F;
constexpr int32_t slagmawSwallowPrompts = 9;
constexpr float slagmawSwallowPromptWindow = 0.7F;
constexpr float slagmawSwallowBeat = 0.3F;
constexpr int32_t slagmawSwallowParryChance = 25;
constexpr int32_t slagmawSwallowBurnDamage = 2; // fail: spat out with one burn hit
constexpr float slagmawPunishWindow = 3.0F;     // clean clear: Slagmaw chokes
constexpr float slagmawPunishDamageMult = 1.5F;
// Kraken Snatch and Dragged Under
constexpr float krakenSnatchRadius = 70.0F;
constexpr float krakenSnatchTell = 1.0F;
constexpr int32_t krakenSnatchCooldownMin = 8; // seconds, rolled per Snatch
constexpr int32_t krakenSnatchCooldownMax = 10;
constexpr float krakenSnatchHold = 0.8F; // plain grab inside the QTE gap
constexpr float krakenDragMashDuration = 1.6F;
constexpr int32_t krakenDragMashPresses = 8;
constexpr float krakenDragMashBeat = 0.5F;
constexpr int32_t krakenDragPrompts = 11;
constexpr int32_t krakenDragPromptsPerPortal = 4; // the ship is hauled through a new portal this often
constexpr float krakenDragPromptWindow = 0.6F;
constexpr float krakenDragBeat = 0.3F;
constexpr int32_t krakenDragParryChance = 30;
// Banished Juggled: Bind's catch, three throws between tentacles
constexpr float banishedJuggleMashDuration = 1.4F;
constexpr int32_t banishedJuggleMashPresses = 9;
constexpr float banishedJuggleMashBeat = 0.5F;
constexpr int32_t banishedJugglePrompts = 13;
constexpr int32_t banishedJugglePromptsPerThrow = 4; // throws start at 13, 9 and 5 left (4, 4, 5)
constexpr float banishedJugglePromptWindow = 0.5F;
constexpr float banishedJuggleBeat = 0.25F;
constexpr int32_t banishedJuggleParryChance = 35;
constexpr float banishedPlainGrabHold = 0.8F;
// Rush QTE: a dodge chain as a charging boss passes (Wreckworm Burrow Charge and Dash Rush, any
// other boss's ChargeDash while enraged)
constexpr float rushQteTriggerDist = 340.0F; // head this close and closing starts it
constexpr float rushQteCrawlSpeed = 420.0F;  // head's real-time crawl, so it passes the ship
constexpr int32_t rushQtePrompts = 3;
constexpr float rushQtePromptWindow = 0.8F;
constexpr float rushQteBeat = 0.3F;
constexpr int32_t rushQteParryChance = 20;

constexpr auto beltbreakerShieldedDuration(int32_t plateCount) -> float
{
    return 12.0F + static_cast<float>(plateCount - 4) * 2.0F;
}
}

struct WaveHitResult
{
    bool resolved;
    bool hit;
};

void triggerShake(Game& game, float intensity, float duration);
void triggerHitPause(Game& game, float duration);
void triggerBossShieldDashShake(Game& game);
void triggerPlayerHitShake(Game& game, float intensity, float duration);
void duckBGM(Game& game);
auto updateTitle(Game& game) -> bool;
auto updateShipSelect(Game& game) -> bool;
auto updatePaused(Game& game) -> bool;
auto updateGameOver(Game& game) -> bool;
auto updateEnding(Game& game, float deltaTime) -> bool;
void tryComboRefund(Game& game);
auto chunkDuration(const Game& game) -> float;
auto dashChunkDuration(const Game& game) -> float;
auto updateLevelUp(Game& game) -> bool;
void playMenuSounds(Game& game, int32_t newIndex, bool confirmed);
void playSFX(Game& game, Sound sound);
auto updateSettings(Game& game) -> bool;
void cycleResolution(Game& game, int32_t dir);
void cycleFPS(Game& game, int32_t dir);
void cycleHudScale(Game& game, int32_t dir);
void gainNerve(Game& game);
void updateNerve(Game& game, float deltaTime);
void updateNerveBurstInput(Game& game);
void fireNerveBurst(Game& game);
void fireNerveLaserStorm(Game& game);
void fireNerveBladeTornado(Game& game);
void fireNerveBall(Game& game);
void sniperLineHit(Game& game, Vector2 start, Vector2 end, int32_t dmg, WeaponType weapon);
void fireLaserWeb(Game& game, const Weapon& weapon, Vector2 nose, Vector2 aim);
void updateDashWedgesAndBlasts(Game& game, float deltaTime);
void updateSniperShots(Game& game, float deltaTime);
void updateOrbitBladeContact(Game& game, float deltaTime);
void updateOrbitBladeLaunch(Game& game, float deltaTime);
void updatePiercingProjectiles(Game& game, float deltaTime,
                               std::vector<OrbitBladeProjectile>& projectiles, DamageSource source,
                               bool despawnOnCameraExit);
void updateOrbitBladeProjectiles(Game& game, float deltaTime);
void updateNerveBallProjectiles(Game& game, float deltaTime);
void updateNerveSpiralProjectiles(Game& game, float deltaTime);
void updateBeamContact(Game& game, float deltaTime);
void damagePlayer(Game& game, int32_t amount);
void spawnDeathExplosion(Game& game);
void updateDeathParticles(Game& game, float deltaTime);
auto updateBossCutscene(Game& game, float deltaTime) -> bool;
auto updateBanishedEntryCutscene(Game& game, float deltaTime) -> bool;
void updateGameplay(Game& game, float deltaTime);
auto resolveExpandingWaveHit(Game& game, Vector2 from, float radius,
                             bool allowDash) -> WaveHitResult;
void updateBossDeathShockwave(Game& game, float deltaTime);
void updatePlayerMovement(Game& game, float deltaTime);
void updateShieldAndBarrier(Game& game, float deltaTime);
void updateAbilityCharges(Game& game, float deltaTime);

struct TouchLayout
{
    Vector2 moveStickAnchor;
    Vector2 aimStickAnchor;
    float stickRadius;
    Vector2 nerveCenter;
    float nerveRadius;
    Vector2 pauseCenter;
    float pauseRadius;
    // Each cluster is 3 separate chamfered-corner button panels - shield and
    // dash stacked vertically side by side, combo as a full-height bar next
    // to them - real distinct filled buttons rather than one outlined shape.
    // Draw reads these same rects (as chamfered rectangles) so the rendered
    // shape and its hit-test can't drift apart.
    float buttonChamfer;
    Rectangle leftShieldRect;
    Rectangle leftComboRect;
    Rectangle leftDashRect;
    Rectangle rightShieldRect;
    Rectangle rightComboRect;
    Rectangle rightDashRect;
};

auto touchControlLayout(const Game& game) -> TouchLayout;
void updateTouchControls(Game& game);
// True on an Android build, or once any touch has been seen this session -
// use this (not TouchControls::visible, which is gameplay-only) to pick
// touch- vs desktop-flavored hint text on any screen.
auto touchInputActive(const Game& game) -> bool;
auto pausePressed(const Game& game) -> bool;
auto dashPressed(const Game& game) -> bool;
auto dashDown(const Game& game) -> bool;
auto shieldPressed(const Game& game) -> bool;
auto shieldDown(const Game& game) -> bool;
auto nerveDown(const Game& game) -> bool;
auto nervePressed(const Game& game) -> bool;
auto nearestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>;
auto farthestEnemy(const Game& game, Vector2 from) -> std::optional<Vector2>;
auto nearestEnemyExcluding(const Game& game, Vector2 from,
                           Vector2 exclude) -> std::optional<Vector2>;
auto weaponCooldown(const Game& game, WeaponType kind, int32_t level, bool evolved) -> float;
auto weaponDamage(const Game& game, int32_t level) -> int32_t;
auto ricochetLevel(const Game& game) -> int32_t;
auto estimatePlayerDps(const Game& game) -> float;
auto dpsHealthScale(const Game& game) -> float;
auto enemyHealthScale(const Game& game) -> float;
auto followerDroneDamageFraction(int32_t level) -> float;
auto laserDroneDamageFraction(int32_t level) -> float;
void updateFollowerDrones(Game& game, float deltaTime);
void updateLaserDrones(Game& game, float deltaTime);
void updateDroneContact(Game& game, float deltaTime);
void updateTurrets(Game& game, float deltaTime);
void updateFlamethrower(Game& game, float deltaTime);
void updateSolarForgeEnemyFireParticles(Game& game, float deltaTime);
void updateChainLightningBolts(Game& game, float deltaTime);
void fireChainLightning(Game& game, const Weapon& weapon);
auto mineCount(int32_t level, bool evolved) -> int;
auto mineRadius(bool evolved) -> float;
auto mineDamage(const Game& game, int32_t level, bool evolved) -> int32_t;
void spawnMines(Game& game, const Weapon& weapon);
void updateMines(Game& game, float deltaTime);
auto nearestEnemyWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
auto nearestAsteroidWithin(const Game& game, Vector2 from, float maxDist) -> std::optional<Vector2>;
void updateWeapons(Game& game, float deltaTime);
void recordDamage(Game& game, DamageSource source, int32_t amount);
auto currentBurnDps(const Game& game) -> float;
void applyElementDebuff(Enemy& enemy, ElementType element, float burnDps);
void applyActiveElementalDebuffs(Game& game, Enemy& enemy);
void applyElementDebuff(Boss& boss, ElementType element, float burnDps);
void applyActiveElementalDebuffs(Game& game, Boss& boss);
void collectElementalPickup(Game& game, ElementType element, ElementMechanism mechanism);
void updatePlayerBuffs(Game& game, float deltaTime);
void aoePulse(Game& game, Vector2 center, float radius, int32_t dmg, DamageSource source,
              float knockback = 0, float bossSlowMult = 1.0F, float bossSlowDuration = 0,
              float projectileRepelForce = 0);
void updateWaveSpawner(Game& game, float deltaTime);
void updateEliteHazards(Game& game, float deltaTime);
void damageEliteHazard(Game& game, size_t index, int32_t amount, bool applyShake = true);
auto enemyDamage(const Game& game, int32_t base) -> int32_t;
void spawnEnemy(Game& game);
void spawnBossWave(Game& game, float healthMult, float sizeMult, bool isMega);
auto sampleBossMoveset(int count) -> std::vector<BossAttack>;
auto spawnRingPosition(const Game& game) -> Vector2;
void updateBossMovement(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void updateTacticTracker(Game& game, float deltaTime);
auto updateBossDirector(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime) -> bool;
auto bossDirectorMove(const Game& game, const Boss& boss, Vector2 bossCenter) -> Vector2;
auto bossAimPoint(const Game& game, const Boss& boss) -> Vector2;
auto bossDirectorDamageMult(const Boss& boss) -> float;
void forceBossPattern(Game& game, Boss& boss, BossPattern pattern);
void updateBeltbreakerCore(Game& game, Boss& core, float deltaTime);
auto countAttachedAlivePlates(Game& game, const Boss& core) -> int32_t;
auto beltbreakerPlateSlotPosition(const Boss& core, int32_t slotIndex) -> Vector2;
auto findBeltbreakerById(Game& game, int32_t instanceId) -> Boss*;
void updateWreckwormChain(Game& game, Boss& head, float deltaTime);
void updateWreckwormSegmentVolley(Game& game, Boss& segment, float deltaTime);
auto bossQteActive(const Game& game) -> bool;
auto bossQteSlowdown(const Game& game) -> float;
enum class QteKind : std::uint8_t
{
    PlateRush,
    Constrict,
    Rush,
    Swallowed,
    DraggedUnder,
    Juggled
};
void bossQteBegin(Game& game, int32_t bossId, QteKind kind);
void bossQteEnd(Game& game);
auto bossQteCanStart(const Game& game, QteKind kind) -> bool;
auto qteSlotFree(const Game& game) -> bool;
void qteBeginFinisher(Game& game);
auto qteUpdateFinisher(Game& game, Vector2 target, float deltaTime) -> bool;
void sniperQteBegin(Game& game, int32_t enemyId, Vector2 aim);
void updateSniperQte(Game& game, float deltaTime);
auto qtePromptForThreat(const Game& game, Vector2 threatDir,
                        int32_t parryChance = UpdateConstants::qteParryChance) -> QtePrompt;
auto qteReadInput(Game& game, QtePrompt prompt) -> int32_t;
void qteStartAction(Game& game, QtePrompt prompt, Vector2 threatDir);
void updateBossQte(Game& game, float deltaTime);
void beginBeltbreakerPlateRush(Game& game, Boss& core);
void updateBeltbreakerPlateRush(Game& game, Boss& core, float deltaTime);
void sendRushPlateHome(Boss& plate);
auto findWreckwormHeadById(Game& game, int32_t instanceId) -> Boss*;
void updateWreckwormConstrict(Game& game, Boss& head, float deltaTime);
void tryBeginRushQte(Game& game, Boss& boss, Vector2 bossCenter);
void updateRushQte(Game& game, Boss& boss, float deltaTime);
void endBossQteClean(Game& game, Boss& boss);
void updateKrakenSnatch(Game& game, Boss& kraken, float deltaTime);
void updateKrakenDraggedUnder(Game& game, Boss& kraken, float deltaTime);
void updateBanishedJuggled(Game& game, Boss& banished, float deltaTime);
auto findBossById(Game& game, int32_t instanceId) -> Boss*;
auto findSlagmawById(Game& game, int32_t instanceId) -> Boss*;
void updateSlagmawSwallowed(Game& game, Boss& slagmaw, float deltaTime);
void updateMagmaPools(Game& game, float deltaTime);
auto slagmawMawCenter(const Boss& slagmaw) -> Vector2;
void updateKrakenTentacle(Game& game, Boss& boss, float deltaTime);
void updateKrakenLimbs(Game& game, Boss& boss, float deltaTime);
auto krakenPodPosition(const Boss& boss, int32_t podIndex) -> Vector2;
auto krakenActiveTentacleCount(int32_t encounter) -> int32_t;
void krakenStaggerSwipingTentacle(Boss& boss);
void spawnKrakenSnake(Game& game, Vector2 nearPos);
void updateKrakenSummon(Game& game, Boss& boss, float deltaTime);
void spawnKraken(Game& game, int32_t wave);
void spawnBanished(Game& game);
void updateBanished(Game& game, Boss& boss, float deltaTime);
void damageBanishedEyeBurst(Game& game, Boss& boss, int32_t divisor);
void spawnSlagmaw(Game& game, int32_t wave);
void spawnSlagmawDrifterBreak(Game& game, Vector2 position, int32_t generation);
void spawnSlagmawHalfBreak(Game& game, const Boss& original, Vector2 bossCenter,
                           std::vector<Boss>& pendingHalves);
void updateSafePathSegments(Game& game, float deltaTime);
void updateFluidHazardContact(Game& game);
void updateGasHazards(Game& game, float deltaTime);
void updateSolarForgeHeat(Game& game, float deltaTime);
void updateRicochetRelay(Game& game, float deltaTime);
void updatePunctumThunder(Game& game, float deltaTime);
void updateDebrisSquall(Game& game, float deltaTime);
void updateRustPatches(Game& game, float deltaTime);
// An enemy or Wreckworm segment died here: blooms or feeds a Rust Patch in Rustbloom.
void bloomRustPatch(Game& game, Vector2 position, bool guaranteed);
struct SquallRock
{
    Vector2 position;
    float radius;
    float seed;
    bool live; // sweeping, on screen and not shot
};
auto debrisSquallRock(const Game& game, int32_t index) -> SquallRock;
// Cheap deterministic 0..1 noise; the Debris Squall uses it in both update and draw so every
// rock sits in the same place for collisions as on screen.
inline auto hashNoise(float seed) -> float
{
    const float v = std::sin(seed) * 43758.5453F;
    return v - std::floor(v);
}
auto findWreckwormSegment(Game& game, int32_t headId, int32_t segmentIndex) -> Boss*;
void triggerWreckwormMolt(Game& game, Boss& head);
void updatePickups(Game& game, float deltaTime);
void applyPickupEffect(Game& game, PickupType type, ElementType element, ElementMechanism mechanism,
                       const Pickup* except = nullptr);
void collectLifeOrb(Game& game);
auto equippedSlotCount(const Game& game) -> int;
auto sampleDistinct(std::vector<SkillType> ids, int count) -> std::vector<SkillType>;
auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>;
auto rollRewardChoices(const Game& game) -> std::vector<LevelUpChoice>;
void applySkill(Game& game, SkillType id);
void grantOrLevelWeapon(Game& game, WeaponType kind);
void applyEvolution(Game& game, WeaponType kind);
void updateBullets(Game& game, float deltaTime);
void damageEnemy(Game& game, size_t index, int32_t amount);
void damageBoss(Game& game, Boss& boss, int32_t amount, bool applyShake = true,
                bool piercesTailWrap = false, bool hitsStaggeredKrakenTentacle = false);
void killEnemyForBossAttack(Game& game, size_t index, bool alwaysLoot);
void spawnRareBonusPickup(Game& game, Vector2 position);
void spawnRareDangerPickup(Game& game, Vector2 position);
void updateAsteroids(Game& game, float deltaTime);
void updateEnemies(Game& game, float deltaTime);
void updateProjectiles(Game& game, float deltaTime);
void filterDeadEntities(Game& game);
void updateBgParticles(Game& game);
void updateBlackHole(Game& game, float deltaTime);
auto spawnBlackHoleDustParticle(const Game& game) -> BlackHoleDustParticle;
void updateBlackHoleDust(Game& game, float deltaTime);
void updateWormhole(Game& game, float deltaTime);
auto applyWormholeTransit(const Game& game, Vector2& position, Vector2& velocity,
                          float entityRadius) -> bool;
void updateBoss(Game& game, float deltaTime, Boss& boss, Vector2 bossCenter);
void startBossAttack(Game& game, Boss& boss, Vector2 bossCenter);
void processBeamAttack(Game& game, Boss& boss, Vector2 beamStart, Vector2 beamEnd);
void updateBgmLayers(Game& game, float deltaTime);

auto nerveFrac(const Game& game) -> float;
auto isNerveChargeFeeding(const Game& game) -> bool;
auto chargeRegenDuration(const Game& game) -> float;
auto aimAtMouse(const Game& game) -> Vector2;
auto mouseWorldPos(const Game& game) -> Vector2;
auto orbitRadius(int32_t level) -> float;
auto orbitBladeCount(int32_t level) -> int32_t;
auto orbitBladePosition(const Game& game, Vector2 center, float radius, int32_t index,
                        int32_t count) -> Vector2;
auto shockwaveRadius(int32_t level, bool evolved) -> float;
auto beamAimDirection(const Game& game, bool evolved) -> Vector2;
auto beamLength(const Game& game, int32_t level, bool evolved) -> float;
auto flamethrowerRayDirs(const Game& game) -> std::array<Vector2, 2>;
auto flamethrowerRayHalfWidth(int32_t level) -> float;
auto waveEnemyScale(const Game& game) -> float;
auto playerConstant(const Game& game) -> float;
auto organicVertexRadius(float baseRadius, int32_t vertexIndex, float seed) -> float;
auto enemyCollisionRadius(const Enemy& enemy) -> float;
void updateOrganicMerges(Game& game, float deltaTime);
auto isFusedPassive(const Game& game, SkillType id) -> bool;
auto hasWeapon(const Game& game, WeaponType kind) -> bool;
auto weaponForGrantSkill(SkillType id) -> std::optional<WeaponType>;
auto weaponEvolved(const Game& game, WeaponType kind) -> bool;
auto bossWindupDuration(BossAttack attack) -> float;
auto wormholeBentBeamSegments(const Game& game, Vector2 start,
                              Vector2 end) -> std::vector<std::pair<Vector2, Vector2>>;

auto isConsumablePickup(PickupType type) -> bool;
auto consumablePickupBiasWeight(int32_t wave) -> float;
auto pickupDurationScale(int32_t wave) -> float;
auto pickWeightedPickupIndex(int32_t wave) -> size_t;
auto damageShakeIntensity(int32_t amount) -> float;
void spawnDamageNumber(Game& game, Vector2 position, int32_t amount);
void updateDamageNumbers(Game& game, float deltaTime);
void updateWeaponDowngrade(Game& game, float deltaTime);
auto updateAchievements(Game& game) -> bool;
auto updateBestiary(Game& game) -> bool;
void spawnKillExplosion(Game& game, Vector2 position, Color color, int32_t particleCount,
                        float speedScale);
void detonateMine(Game& game, size_t index);
auto isOutsideCameraView(const Game& game, Vector2 pos, float margin) -> bool;
void spawnRockCluster(Game& game, Vector2 center);
void spawnBossInstance(Game& game, float healthMult, float sizeMult, bool isMega, bool isSwarm);
auto beltbreakerCoreCenter(const Boss& core) -> Vector2;
auto countContributingPlates(Game& game, const Boss& core) -> int32_t;
void detachBeltbreakerPlates(Game& game, const Boss& core);
void triggerBeltbreakerReturn(Game& game, const Boss& core);
void sendPlateOnExcursion(Boss& plate, float durationSeconds);
auto demoSkillAllowed(const Game& game, SkillType id) -> bool;
auto droneCountForLevel(int32_t level) -> size_t;

auto UpdateGame(Game& game, float deltaTime) -> bool;

void applyBGMState(Game& game);
void startLevelUp(Game& game);
void spawnBoss(Game& game);
void spawnMiniboss(Game& game);
void spawnBeltbreaker(Game& game, int32_t wave);
void spawnWreckworm(Game& game, int32_t wave);
void spawnSwarmBoss(Game& game);
void spawnEnemyAt(Game& game, int kindIndex, Vector2 pos);
void forceBossAttack(Game& game, Boss& boss, BossAttack attack);
void spawnBlackHole(Game& game);
void spawnWormholePair(Game& game);
void spawnEliteHazard(Game& game, EliteHazardRole role);
void spawnPickup(Game& game, Vector2 position, int value, PickupType type,
                 ElementType element = ElementType::Static,
                 ElementMechanism mechanism = ElementMechanism::Infusion);

auto isWeaponTypeUnlocked(const Game& game, WeaponType weapon) -> bool;
auto isEvolutionUnlocked(const Game& game, WeaponType weapon) -> bool;
auto weaponSlotCap(const Game& game) -> int32_t;
void recordWeaponKill(Game& game, WeaponType weapon);
void recordSkillMaxed(Game& game, SkillType id);
void recordDashKill(Game& game, int32_t enemyKindIndex);
void recordEnemyKilled(Game& game, int32_t enemyKindIndex);
void recordBossKilled(Game& game, const Boss& boss);
void recordHazardKilled(Game& game, EliteHazardRole role);
void recordDashOrNerveKill(Game& game);
void recordWaveReached(Game& game, int32_t wave);
