#include "update_internal.hpp"

auto isConsumablePickup(PickupType type) -> bool { return type == PickupType::SecondWind; }

auto consumablePickupBiasWeight(int32_t wave) -> float
{
    constexpr float maxBias = 4.0F;
    constexpr float waveDivisor = 25.0F;
    return std::min(maxBias, 1.0F + static_cast<float>(wave) / waveDivisor);
}

auto pickupDurationScale(int32_t wave) -> float
{
    constexpr float maxBonus = 0.5F;
    constexpr float waveDivisor = 200.0F;
    return 1.0F + std::min(maxBonus, static_cast<float>(wave) / waveDivisor);
}

auto pickWeightedPickupIndex(int32_t wave) -> size_t
{
    std::array<float, pickupCatalog.size()> weights{};
    float total = 0.0F;
    for (size_t i = 0; i < pickupCatalog.size(); i++)
    {
        weights.at(i) =
            isConsumablePickup(pickupCatalog.at(i).type) ? consumablePickupBiasWeight(wave) : 1.0F;
        total += weights.at(i);
    }

    constexpr int32_t rollResolution = 100000;
    float roll = static_cast<float>(GetRandomValue(0, rollResolution)) /
                 static_cast<float>(rollResolution) * total;
    for (size_t i = 0; i < weights.size(); i++)
    {
        if (roll < weights.at(i))
        {
            return i;
        }
        roll -= weights.at(i);
    }
    return weights.size() - 1;
}

void updatePickups(Game& game, float deltaTime)
{
    const float magnetRadius = 110 +
                               110 *
                                   static_cast<float>(game.run.skillLevels.at(
                                       static_cast<size_t>(SkillType::PickupRadius))) *
                                   0.2F +
                               pickupRadiusPerLevel * static_cast<float>(game.run.level - 1);

    for (auto& pickup : game.run.pickups)
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

        if (game.run.blackhole.active &&
            Vector2Distance(pickup.position, game.run.blackhole.position) <=
                game.run.blackhole.radius)
        {
            pickup.active = false;
            continue;
        }

        const Vector2 toPlayer = Vector2Subtract(game.run.player.position, pickup.position);
        const float dist = Vector2Length(toPlayer);

        // Danger is never pulled: the player has to fly into it.
        const bool pulled =
            pickup.type != PickupType::Danger && (pickup.homing || dist <= magnetRadius);
        pickup.pull = std::clamp(
            pickup.pull + deltaTime * (pulled ? pickupPullRampUp : -pickupPullRampDown), 0.0F, 1.0F);
        if (pickup.pull > 0 && dist > 0)
        {
            const float ease = pickup.pull * pickup.pull;
            float speed = pickupMagnetSpeed * frameScale *
                          (pickupMagnetMinFrac + (pickupMagnetMaxFrac - pickupMagnetMinFrac) * ease);
            if (pickup.homing)
            {
                speed = std::max(speed, dist * pickupHomingRate * ease);
            }
            pickup.position = Vector2Add(
                pickup.position, Vector2Scale(toPlayer, std::min(1.0F, speed * deltaTime / dist)));
        }

        if (dist <= game.run.player.radius + 6)
        {
            pickup.active = false;

            if (pickup.type == PickupType::XP)
            {
                game.run.xp += pickup.value;
            }
            else
            {
                applyPickupEffect(game, pickup.type, pickup.element, pickup.mechanism, &pickup);
            }
        }
    }
}

void applyPickupEffect(Game& game, PickupType type, ElementType element, ElementMechanism mechanism,
                       const Pickup* except)
{
    switch (type)
    {
    case PickupType::XP:
        break;
    case PickupType::LifeOrb:
        collectLifeOrb(game);
        break;
    case PickupType::Shield:
        if (game.run.player.shieldStacks < currentShip(game).maxShieldStacks)
        {
            game.run.player.shieldStacks++;
        }
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Elemental:
        collectElementalPickup(game, element, mechanism);
        break;
    case PickupType::Regen:
        game.run.player.regenTimer =
            pickupEffectDuration * pickupDurationScale(game.run.waveNumber);
        game.run.player.regenRate = regenRate;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::MagnetPulse:
        for (auto& other : game.run.pickups)
        {
            if (other.active && &other != except && other.type != PickupType::Danger)
            {
                other.homing = true;
                // long enough for the flight in, however close it was to expiring
                other.lifetime = std::max(other.lifetime, 4.0F);
            }
        }
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Overcharge:
        game.run.player.overchargeTimer =
            overchargeDuration * pickupDurationScale(game.run.waveNumber);
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::SecondWind:
        game.run.player.secondWindReady = true;
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Overdrive:
        game.run.player.overdriveTimer =
            overdriveDuration * pickupDurationScale(game.run.waveNumber);
        playSFX(game, game.resources.sounds.menuConfirm);
        break;
    case PickupType::Danger:
        if (GetRandomValue(0, 1) == 0)
        {
            game.run.player.blindTimer = blindDuration;
            game.run.achievementToast = "Blinded!";
        }
        else
        {
            game.run.player.frenzyTimer = frenzyDuration;
            game.run.achievementToast = "Enemy frenzy!";
        }
        game.run.achievementToastTimer = 3.0F;
        triggerShake(game, 6, 0.25F);
        playSFX(game, game.resources.sounds.explosion);
        break;
    case PickupType::Count:
        break;
    }
}

void collectElementalPickup(Game& game, ElementType element, ElementMechanism mechanism)
{
    const float burnDps = currentBurnDps(game);

    switch (mechanism)
    {
    case ElementMechanism::Infusion:
        game.run.player.elementalBuffTimer.at(static_cast<size_t>(element)) =
            pickupEffectDuration * pickupDurationScale(game.run.waveNumber);
        break;
    case ElementMechanism::Nova:
        for (auto& enemy : game.run.enemies)
        {
            if (enemy.active && !enemy.phased)
            {
                applyElementDebuff(enemy, element, burnDps);
            }
        }
        for (auto& boss : game.run.bosses)
        {
            if (boss.health > 0)
            {
                applyElementDebuff(boss, element, burnDps);
            }
        }
        break;
    case ElementMechanism::Count:
        break;
    }

    playSFX(game, game.resources.sounds.menuConfirm);
}

void startLevelUp(Game& game)
{
    game.run.xp -= game.run.xpToNext;
    game.run.level++;
    game.run.xpToNext = 100 + (game.run.level - 1) * 60;

    game.run.player.maxHealth += maxHealthPerLevel;
    game.run.player.health += maxHealthPerLevel;

    game.run.pendingChoices = rollLevelUpChoices(game);
    game.menuIndex = 0;
    game.state = GameState::LEVEL_UP;
    playSFX(game, game.resources.sounds.menuConfirm);
}

auto isFusedPassive(const Game& game, SkillType id) -> bool
{
    return std::ranges::any_of(
        game.run.weapons, [&](const Weapon& w)
        { return w.evolved && skillLinkedPassive.at(static_cast<size_t>(w.type)) == id; });
}

auto equippedSlotCount(const Game& game) -> int
{
    int count = 0;
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.run.skillLevels.at(i) > 0 && !isFusedPassive(game, id))
        {
            count++;
        }
    }
    return count;
}

auto hasWeapon(const Game& game, WeaponType kind) -> bool
{
    return std::ranges::any_of(game.run.weapons,
                               [kind](const Weapon& w) { return w.type == kind; });
}

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

auto demoSkillAllowed(const Game& game, SkillType id) -> bool
{
    if (!isPassiveUnlocked(game, id))
    {
        return false; // a helper not yet bought in the skill tree
    }
    for (size_t i = 0; i < weaponGrantSkill.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        if (weaponGrantSkill.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(kind) && isWeaponTypeUnlocked(game, kind);
        }
        if (skillLinkedPassive.at(i) == id)
        {
            return DemoConfig::isWeaponAllowed(kind);
        }
    }
    return true;
}

auto rollLevelUpChoices(Game& game) -> std::vector<LevelUpChoice>
{
    const bool slotsFull = equippedSlotCount(game) >= weaponSlotCap(game);

    std::vector<SkillType> eligible;
    eligible.reserve(static_cast<size_t>(SkillType::Count));
    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (isFusedPassive(game, id) || game.run.skillLevels.at(i) >= Skills.at(i).maxLevel ||
            !demoSkillAllowed(game, id))
        {
            continue;
        }
        if (game.run.skillLevels.at(i) > 0 || !slotsFull)
        {
            eligible.push_back(id);
        }
    }

    std::vector<LevelUpChoice> choices;
    if (eligible.empty())
    {
        choices = rollRewardChoices(game);

        if (game.run.postCapDamageLevels < postCapDamageLevelsCap)
        {
            game.run.postCapDamageLevels++;
        }
    }
    else
    {
        std::vector<SkillType> owned;
        for (const auto id : eligible)
        {
            if (game.run.skillLevels.at(static_cast<size_t>(id)) > 0)
            {
                owned.push_back(id);
            }
        }

        std::vector<SkillType> picks;
        const bool affinityTriggers = !owned.empty() && GetRandomValue(0, 99) < 50;
        if (affinityTriggers)
        {
            picks = sampleDistinct(owned, 1);
            std::vector<SkillType> rest;
            rest.reserve(eligible.size());
            for (const auto id : eligible)
            {
                if (id != picks.front())
                {
                    rest.push_back(id);
                }
            }
            const auto more = sampleDistinct(rest, 2);
            picks.insert(picks.end(), more.begin(), more.end());
        }
        else
        {
            picks = sampleDistinct(eligible, 3);
        }

        for (const auto id : picks)
        {
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = id});
        }

        while (choices.size() < 3 && !eligible.empty())
        {
            const auto extra = sampleDistinct(eligible, 1);
            choices.push_back(LevelUpChoice{.type = ChoiceType::Skill, .skill = extra.front()});
        }
    }

    std::vector<WeaponType> evolvable;
    for (size_t i = 0; i < weaponEvolves.size(); i++)
    {
        if (!weaponEvolves.at(i))
        {
            continue;
        }
        const auto kind = static_cast<WeaponType>(i);
        const auto grantSkill = weaponGrantSkill.at(i);
        const auto passive = skillLinkedPassive.at(i);
        if (hasWeapon(game, kind) && !weaponEvolved(game, kind) &&
            game.run.skillLevels.at(static_cast<size_t>(grantSkill)) >= 3 &&
            game.run.skillLevels.at(static_cast<size_t>(passive)) >= 3 &&
            isEvolutionUnlocked(game, kind))
        {
            evolvable.push_back(kind);
        }
    }
    if (!evolvable.empty() && !choices.empty())
    {
        const auto pick = evolvable.at(
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(evolvable.size()) - 1)));
        const auto replaceIndex =
            static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(choices.size()) - 1));
        choices.at(replaceIndex) = LevelUpChoice{.type = ChoiceType::Evolve, .weapon = pick};
    }

    for (size_t i = choices.size(); i > 1; i--)
    {
        const auto j = static_cast<size_t>(GetRandomValue(0, static_cast<int32_t>(i) - 1));
        std::swap(choices.at(i - 1), choices.at(j));
    }

    const auto& dropEntry = pickupCatalog.at(pickWeightedPickupIndex(game.run.waveNumber));
    choices.push_back(LevelUpChoice{.type = ChoiceType::Pickup,
                                    .pickupType = dropEntry.type,
                                    .element = dropEntry.element,
                                    .mechanism = dropEntry.mechanism});

    return choices;
}

auto rollRewardChoices(const Game& game) -> std::vector<LevelUpChoice>
{
    std::vector<size_t> picked;
    while (picked.size() < 3 && picked.size() < pickupCatalog.size())
    {
        const size_t idx = pickWeightedPickupIndex(game.run.waveNumber);
        if (std::ranges::find(picked, idx) == picked.end())
        {
            picked.push_back(idx);
        }
    }

    std::vector<LevelUpChoice> choices;
    for (const auto idx : picked)
    {
        const auto& entry = pickupCatalog.at(idx);
        choices.push_back(LevelUpChoice{.type = ChoiceType::Pickup,
                                        .pickupType = entry.type,
                                        .element = entry.element,
                                        .mechanism = entry.mechanism});
    }
    return choices;
}

auto weaponEvolved(const Game& game, WeaponType kind) -> bool
{
    for (const auto& w : game.run.weapons)
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
    game.run.skillLevels.at(static_cast<size_t>(id))++;

    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        grantOrLevelWeapon(game, *kind);
        if (game.run.skillLevels.at(static_cast<size_t>(id)) >=
            Skills.at(static_cast<size_t>(id)).maxLevel)
        {
            recordSkillMaxed(game, id);
        }
        return;
    }

    switch (id)
    {
    case SkillType::MoveSpeed:
        game.run.player.speed *= 1.1F;
        break;
    case SkillType::MaxHp:
        game.run.player.maxHealth++;
        game.run.player.health = game.run.player.maxHealth;
        break;
    default:

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
    for (auto& w : game.run.weapons)
    {
        if (w.type == kind)
        {
            w.level++;
            return;
        }
    }
    game.run.weapons.push_back(Weapon{.type = kind, .level = 1});
}

void applyEvolution(Game& game, WeaponType kind)
{
    for (auto& w : game.run.weapons)
    {
        if (w.type == kind)
        {
            w.evolved = true;
            triggerShake(game, 10, 0.3F);
            playSFX(game, game.resources.sounds.critical);
            return;
        }
    }
}

void spawnPickup(Game& game, Vector2 position, int value, PickupType type, ElementType element,
                 ElementMechanism mechanism)
{
    const float lifetime = type == PickupType::XP ? xpPickupLifetime : bonusPickupLifetime;
    game.run.pickups.push_back(Pickup{.position = position,
                                      .value = value,
                                      .type = type,
                                      .element = element,
                                      .mechanism = mechanism,
                                      .active = true,
                                      .lifetime = lifetime,
                                      .maxLifetime = lifetime});
}

void spawnRareBonusPickup(Game& game, Vector2 position)
{
    if (GetRandomValue(0, 999) >= rareBonusDropChance)
    {
        return;
    }

    switch (GetRandomValue(0, rareBonusCategoryCount - 1))
    {
    case 0:
        spawnPickup(game, position, 0, PickupType::Regen);
        break;
    case 1:
        spawnPickup(game, position, 0, PickupType::MagnetPulse);
        break;
    case 2:
        spawnPickup(game, position, 0, PickupType::Overcharge);
        break;
    case 3:
        spawnPickup(game, position, 0, PickupType::SecondWind);
        break;
    default:
    {
        const auto element = static_cast<ElementType>(
            GetRandomValue(0, static_cast<int32_t>(ElementType::Count) - 1));
        const auto mechanism = static_cast<ElementMechanism>(
            GetRandomValue(0, static_cast<int32_t>(ElementMechanism::Count) - 1));
        spawnPickup(game, position, 0, PickupType::Elemental, element, mechanism);
        break;
    }
    }
}

void spawnRareDangerPickup(Game& game, Vector2 position)
{
    if (GetRandomValue(0, 999) >= dangerPickupChance)
    {
        return;
    }
    spawnPickup(game, position, 0, PickupType::Danger);
}

auto currentBurnDps(const Game& game) -> float
{
    return baseBurnDps * enemyHealthMult * waveEnemyScale(game);
}
