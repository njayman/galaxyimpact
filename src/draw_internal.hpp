#pragma once

// Declarations shared by the draw_*.cpp files. Not for use outside drawing code.

#include "draw.hpp"

#include "bestiary.hpp"
#include "entities/boss.hpp"
#include "entities/enemy.hpp"
#include "entities/item.hpp"
#include "entities/player.hpp"
#include "entities/ship.hpp"
#include "entities/space.hpp"
#include "guitheme.hpp"
#include "punctum_infall.hpp"
#include "menu.hpp"
#include "palette.hpp"
#include "progression.hpp"
#include "raygui.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include "sandbox.hpp"
#include "settings.hpp"
#include "ships/bastion.hpp"
#include "ships/interceptor.hpp"
#include "ships/ranger.hpp"
#include "update.hpp"
#include "update_constants.hpp"
#include <algorithm>
#include <span>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <limits>
#include <numbers>
#include <optional>
#include <string>
#include <vector>

namespace DrawDetail
{

inline constexpr float transitionCameraPush = 0.08F;

inline const Color wreckwormSlime = ColorLerp(Palette::RustbloomAccent, Palette::RustbloomVoid, 0.5F);


void drawTitle(const Game& game);
void drawEnemyShape(EnemyShape shape, Vector2 center, float radius, float facingDeg, Color color, float seed = 0);
void drawEliteHazardShape(EliteHazardRole role, Vector2 center, float radius, Color body, Color signal);
void drawEliteHazardAura(Vector2 center, float radius, Color signal, float turn);
void drawBossHull(BossShape shape, Vector2 center, Vector2 size, Color color);
void drawShipSelect(const Game& game);
void drawGameOver(const Game& game);
void drawEnding(const Game& game);
void drawOverlay(const Game& game);
auto easeOutBack(float t) -> float;
auto popScale(const Game& game, int32_t index) -> float;
void beginPop(Vector2 center, float scale);
void endPop();
void drawSettings(const Game& game);
void drawSkillIcon(SkillType id, Vector2 c, float s, Color color, bool evolved = false);
void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color, bool evolved = false);
void drawPassiveIcon(SkillType id, Vector2 c, float s, Color color);
auto evolutionHint(const Game& game, SkillType id) -> std::string;
void drawAbilitySlots(const Game& game, int32_t x, int32_t y, bool hover = true);
void drawLevelUp(const Game& game);
void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y);
void drawHighScores(const Game& game, int32_t x, int32_t y, const std::vector<int32_t>& scores);
void drawGameplayWorld(const Game& game);
void drawPunctumLightning(const Game& game);
void drawDebrisSquall(const Game& game);
void drawRustPatches(const Game& game);
void drawShipHull(ShipClass shipClass, Vector2 p, float r, float angle, Color shipColor);
void drawShip(const Game& game);
void drawEngineFlame(Vector2 p, float r, float angle);
void drawEnemy(const Game& game, const Enemy& enemy, bool buffed);
void drawEliteHazard(const EliteHazard& hazard, Color body);
void drawBoss(const Game& game, const Boss& boss);
void drawOrbitBlades(const Game& game);
void drawOrbitBladeProjectiles(const Game& game);
void drawNerveBallProjectiles(const Game& game);
void drawNerveSpiralProjectiles(const Game& game);
void drawShieldIndicator(const Player& player);
void drawChargeParticles(Vector2 center, float fraction, Color color, float maxRadius,
                         float minRadius);
void drawComet(const BossProjectile& projectile);
void drawMeteorProjectile(const BossProjectile& projectile);
void drawSporeProjectile(const BossProjectile& projectile);
void drawKrakenHurledEnemy(const BossProjectile& projectile);
void drawFluidBlob(Vector2 center, float radius, Color color, float seed, Vector2 trailDir);
void drawLavaChunk(Vector2 center, float radius, float seed, Vector2 velocity);
void drawWreckwormHeadShape(Vector2 center, float radius, float seed, Color color, Vector2 facing, float open);
void drawKrakenShape(Vector2 center, float radius, float seed, Color color, Vector2 facing = Vector2{.x = 0, .y = 1},
                     float life = 1.0F, Color signal = Palette::ElementFreeze);
void drawChain(std::span<const Vector2> pts, float rootWidth, float tipWidth, Color a, Color b);
void plateRing(Vector2 center, float inner, float outer, int32_t plates, float rotDeg, float gapDeg, Color color);
void drawTearPortal(Vector2 center, float radius, float open);
void flatTriangle(Vector2 a, Vector2 b, Vector2 c, Color color);
void drawBendyTentacle(Vector2 anchor, Vector2 tip, Vector2 bodyCenter, float bodyRadius,
                       float seed, float thicknessScale, float waveScale, bool calm, Color color,
                       Color tipColor);
void drawKrakenTentacle(const Boss& boss, int32_t slot);
void drawKrakenSnatch(const Boss& boss);
void drawBossQteHud(const Game& game);
void drawKrakenStaggeredTentacle(const Boss& boss, int32_t slot);
void drawKrakenLimb(const Boss& boss, int32_t slot);
void drawKrakenGrabbedEnemy(Vector2 pos, int32_t kindIndex, float scale, float facingAngle);
void drawBanishedShape(Vector2 center, const Boss& boss, Vector2 ship);
void drawBanishedTentacle(const Boss& boss, int32_t slot);
void drawBanishedRestingArms(Vector2 center, const Boss& boss);
void drawBanishedToss(const Boss& boss, Vector2 ship);
void drawBanishedHurled(const BossProjectile& projectile);
// Shared beam look. Charge: energy gathers at `at` (progress 0..1; lock 0..1 as the aim freezes).
// Beam: life 0..1 over its firing time opens it, holds it and pinches it shut; life < 0 holds it open.
void drawBeamCharge(Vector2 at, float radius, float progress, float lock, Color color);
void drawBeam(Vector2 from, Vector2 to, float width, float life, Color color);
void drawBanishedEye(const Boss& boss, Vector2 lookAt);
void drawBanishedBody(Vector2 center, float size, float seed, float reveal, uint32_t brokenSeals, float eyeGlow,
                      float flash, float facing);
void drawBanishedStub(Vector2 center, float size, float instanceSeed, int32_t slot, float reach, Color color);
void drawBanishedRealm(const Game& game);
void drawRealmTunnel(const Game& game);
// Banished realm palette (docs/design/biomes.md)
inline constexpr Color banishedVoid{.r = 14, .g = 10, .b = 20, .a = 255};
inline constexpr Color banishedBone{.r = 207, .g = 200, .b = 184, .a = 255};
inline constexpr Color banishedFlesh{.r = 40, .g = 31, .b = 56, .a = 255};
inline constexpr Color banishedTendril{.r = 78, .g = 62, .b = 104, .a = 255};
// Entry cutscene staging: the husk hangs at the realm's centre, the ship arrives below it.
inline constexpr float krakenHuskRadius = 110.0F;
void drawPanelShape(Vector2 center, float radius, int32_t sides, float rotationDeg, Color baseColor,
                    float seed);
void drawPanelHex(Vector2 center, float radius, float rotationDeg, Color baseColor, float seed);
void drawHUD(const Game& game);
void drawTouchControls(const Game& game);
void drawStatusRow(const Game& game, int32_t x, int32_t y);
auto drawVitals(const Game& game, int32_t x, int32_t y) -> int32_t;
auto withThousands(int32_t value) -> std::string;
auto bossLineColor(const Boss& boss) -> Color;
void drawOffscreenBosses(const Game& game, int32_t width, int32_t height);
void drawRunNumbers(const Game& game, int32_t right, int32_t y);
void drawBossLines(const Game& game, int32_t width, int32_t height);
void drawOrganicEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawHuskSentinelShape(Vector2 center, float radius, float seed, Vector2 aim, Color color);
void drawCarrionSpots(Vector2 center, float radius, float seed, Color color);
void drawClusterEnemyShape(Vector2 center, float baseRadius, float seed, Color color);
void drawMeteorBossShape(Vector2 center, float radius, int32_t seed, Color baseColor);
void drawLavaMole(const Game& game, const Boss& boss, Vector2 center, float flash);
void drawSlagGob(Vector2 position, float radius, Vector2 velocity);
void drawMagmaPools(const Game& game);
void drawSandboxToggleIndicator(const Game& game);
void drawBossDirectorTells(const Boss& boss, Vector2 center);
void drawSandboxMenu(const Game& game);
void drawSandboxBrowser(const Game& game);
void drawLaserBolt(const Bullet& b);
void drawPickupIcon(PickupType type, ElementType element, Vector2 position, float alpha);
void drawWormholeMouth(Vector2 position, WormholeFacing facing, float radius, Color rim);
void beginPixelZoom(const Game& game);
void beginWorldCamera(const Game& game);
auto tiledWorldPos(Vector2 relativeTo, Vector2 offset, float tileW, float tileH) -> Vector2;
auto biomeVoidColor(const Game& game) -> Color;
auto biomeHazeColor(const Game& game) -> Color;
void drawBackgroundStars(const Game& game, Vector2 relativeTo);
void windowText(const Font& font, const Game& game, const char* text, int32_t centerX, int32_t y, int32_t size, Color color);
void windowText(const Game& game, const char* text, int32_t centerX, int32_t y, int32_t size, Color color);
auto easeOutCubic(float t) -> float;
auto overGameplay(const Game& game, GameState state) -> bool;
auto shipWindowPos(const Game& game) -> Vector2;
void updateTransitions(Game& game);
auto irisMaxRadius(const Game& game, Vector2 anchor) -> float;
auto transitionPush(const Game& game) -> float;
void drawIrisRim(Vector2 anchor, float radius, float progress);
void drawIrisReveal(const Game& game);
void drawIrisClose(const Game& game);
void drawAchievements(Game& game);
void drawSkillTree(const Game& game);
void drawWeaponGallery(const Game& game);
auto bestiarySectionHeight(const Game& game, float scale, size_t count) -> float;
auto bestiaryContentHeight(const Game& game, float scale) -> float;
void drawBestiary(Game& game);
auto elementEffectSummary(ElementType element) -> std::string_view;
auto pickupChoiceDescription(PickupType type, ElementType element, ElementMechanism mechanism) -> std::string;
void drawBrowserParticleDemo(int32_t style, Vector2 center);
void drawSolarForgeCaveBackground(const Game& game);
void drawBeltPlanet(const Game& game);
void drawRustbloomDerelict(Vector2 center, float angle, float length, float seed, const std::function<Color(Color, float)>& tone);
void drawRustbloomDerelicts(const Game& game);
void drawRustbloomBackground(const Game& game);
void drawSquallRockFlame(Vector2 pos, Vector2 along, Vector2 across, float radius, float seed);
void drawSquallRock(Vector2 pos, Vector2 along, float radius, float rotationDeg, float seed);
void drawBlackHoleGradient(Vector2 center, float radius);
void drawBlackHoleTwirl(Vector2 center, float radius, const std::vector<BlackHoleDustParticle>& dust);
void drawKrakenRemains(const Game& game);
void drawPlayerBullet(const Bullet& b);
void drawOrbitRingBlade(Vector2 pos, Color color, bool evolved);
void drawOrbitShot(Vector2 pos, float radius, bool evolved);
void drawNerveBall(Vector2 pos, float radius, bool evolved);
void drawSpiralBlade(Vector2 pos, bool evolved);
void drawElementalDebuffEffects(Vector2 pos, float radius, bool burning, bool shocked, bool frozen);
void drawWreckwormGooBridge(Vector2 from, Vector2 to, float fromRadius, float toRadius, float seed);
void drawWreckwormSegmentGun(Vector2 center, float radius, Vector2 aim, float ext, bool flash);
void drawWreckwormSegmentShape(Vector2 center, float radius, float seed, Color baseColor, bool armor, float spinDeg, Vector2 spineDir);
void drawKrakenPortal(Vector2 portal, float t, float open = 1.0F);
void drawKrakenRope(const std::array<Vector2, Boss::krakenRopePoints>& pts, float rootWidth, float tipWidth, float alpha);
void drawBanishedPortal(Vector2 position, float progress);
void drawQteArrow(Vector2 center, Vector2 dir, float size, Color color);
void drawQteKeycap(const Game& game, const char* label, Vector2 center, float width, float press, Color color);
void drawQteMouse(Vector2 center, bool left, bool right, float press, Color color);
void drawQteTouchButton(const Game& game, const char* label, Vector2 center, float press, Color color);
void drawQteButtons(const Game& game, QtePrompt prompt, Vector2 center, float press, Color color);
auto hudCalmAlpha(const Game& game) -> float;
void drawStatusRing(Vector2 center, float radius, float frac, Color color);

} // namespace DrawDetail
