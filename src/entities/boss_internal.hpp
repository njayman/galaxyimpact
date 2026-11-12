#pragma once

// Declarations shared by the files split out of src/entities/boss.cpp; not a public interface.

#include "update.hpp"
#include "progression.hpp"
#include "democonfig.hpp"
#include "entities/ship.hpp"
#include "raymath.h"
#include "update_constants.hpp"
#include <algorithm>
#include <cmath>
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

void seedWreckwormTrail(Boss& head, Vector2 headCenter, Vector2 trailDir);
void beginWreckwormConstrict(Game& game, Boss& head);
void updateLavaMole(Game& game, Boss& boss, Vector2 bossCenter, float deltaTime);
void moleBreach(Game& game, Boss& boss, Vector2 at, float radius);
void slagmawErupt(Game& game, Boss& boss);
void shedPetals(Game& game, Boss& boss, Vector2 bossCenter);
void dropMagmaTrail(Game& game, Boss& boss, Vector2 bossCenter, Vector2 step);
void endKrakenDraggedUnder(Game& game, Boss& kraken, bool clean);
auto wreckwormCoilSegmentDeg(const Boss& head, int32_t segmentIndex) -> float;

auto isGenericMovesetAttack(BossAttack attack) -> bool;
auto steerHeading(Vector2 heading, Vector2 desired, float maxTurn) -> Vector2;
auto bossQteStunned(const Boss& boss) -> bool;
void endRushQte(Game& game, Boss& boss, bool clean);
auto countContributingPlates(Game& game, const Boss& core) -> int32_t;
auto ringCrushDuration(const Boss& core) -> float;
void updateRingCrush(Game& game, Boss& core);
auto beltbreakerRushOffscreenDist(const Game& game) -> float;
auto isRushPlateOf(const Boss& plate, const Boss& core) -> bool;
void endBeltbreakerPlateRush(Game& game, Boss& core, bool clean);
void launchBeltbreakerRushAttack(Game& game, Boss& core, Boss& plate);
void resolveBeltbreakerRushHit(Game& game, Boss& plate);
void endWreckwormConstrict(Game& game, Boss& head, bool clean);
void launchWreckwormConstrictStrike(Game& game, Boss& head);
auto wreckwormDashRushOffDist(const Game& game) -> float;
auto wreckwormDashRushTravelDuration(const Game& game, int32_t segmentCount) -> float;
auto pointAlongWreckwormTrail(Vector2 headCenter, const std::vector<Vector2>& trail, float dist) -> Vector2;
void moveWreckwormSegmentToward(Boss& seg, Vector2 targetCenter, float speed, float deltaTime);
void beginSlagmawSwallowed(Game& game, Boss& slagmaw);
void endSlagmawSwallowed(Game& game, Boss& slagmaw, bool clean);
void throwSlagRing(Game& game, Vector2 at, int32_t count);
auto findMagmaBlob(Vector2 near, float minDist, float maxDist) -> Vector2;
auto krakenStaggerDuration(int32_t encounter) -> float;
auto krakenTentacleSlotIdle(const Boss& boss, int32_t slot) -> bool;
auto krakenBodyCenter(const Boss& boss) -> Vector2;
void seedKrakenRope(Boss& boss, int32_t rope, Vector2 root, Vector2 dir, float length);
void stepKrakenRope(Boss& boss, int32_t rope, Vector2 root, Vector2 target, float length, float pull, float deltaTime);
auto krakenRopeHits(const Boss& boss, int32_t rope, Vector2 point, float radius) -> bool;
auto krakenLashRoot(const Boss& boss, int32_t slot) -> Vector2;
void staggerKrakenSlot(Boss& boss, int32_t slot);
void startKrakenArmAttack(Game& game, Boss& boss, int32_t slot, int32_t kind);
void updateKrakenSlot(Game& game, Boss& boss, int32_t slot, float deltaTime);
void releaseKrakenGrip(Boss& kraken);
void flingFromKrakenGrip(Game& game, Boss& kraken);
void beginKrakenDraggedUnder(Game& game, Boss& kraken);
auto krakenFeedRest(const Boss& kraken, float sign) -> Vector2;
void beginBanishedCoil(Boss& boss, Player& player, int32_t idx, Vector2 contactPoint);
void resolveBanishedCoil(Game& game, Boss& boss, int32_t idx);
void gripBanishedTentacle(Boss& boss, int32_t slot, Vector2 tip);
void releaseBanishedTentacle(Boss& boss);
void beginBanishedJuggled(Game& game, Boss& boss, int32_t slot);
void endBanishedJuggled(Game& game, Boss& boss, bool clean);
