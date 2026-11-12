#include "draw_internal.hpp"

namespace DrawDetail
{

void drawSettings(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "SETTINGS", game.resources.windowWidth / 2, 130, 34, Palette::Accent);

    const auto& res =
        resolutionOptions.at(static_cast<size_t>(game.resources.settings.resolutionIndex));
    const std::string displayMode = IsWindowFullscreen() ? "Fullscreen" : "Windowed";

    const std::array<std::string, 7> stepperLabels{"Resolution",   "", "", "FPS Cap", "HUD Scale",
                                                   "Display Mode", ""};
    const std::array<std::string, 7> stepperValues{
        std::format("{}x{}", res.width, res.height),
        "",
        "",
        std::format("{}", fpsOptions.at(static_cast<size_t>(game.resources.settings.fpsIndex))),
        std::format("{:.0f}%",
                    hudScaleOptions.at(static_cast<size_t>(game.resources.settings.hudScaleIndex)) *
                        100),
        displayMode,
        "",
    };

    const int32_t rowCount = settingsRows(game);
    for (int32_t i = 0; i < rowCount; i++)
    {
        const Rectangle rect =
            menuColumnRect(game, i, rowCount, MenuLayout::settingsWidth, MenuLayout::settingsHeight,
                           MenuLayout::settingsGap, MenuLayout::settingsMenuY);

        if (i == game.menuIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }

        const int32_t row = settingsRowSetting(game, i);
        if (row == 1 || row == 2 || row == 6)
        {
            const bool on = row == 1   ? game.resources.settings.bgmOn
                            : row == 2 ? game.resources.settings.soundOn
                                       : game.resources.settings.swapTouchSticks;
            const char* label = row == 1 ? "BGM" : row == 2 ? "Sound" : "Swap Sticks";
            uiButton(rect, std::format("{}:   {}", label, on ? "ON" : "OFF").c_str(), false);
        }
        else if (i == rowCount - 1)
        {
            uiButton(rect, "Back", false);
        }
        else
        {
            const std::string text =
                std::format("{}:   <  {}  >", stepperLabels.at(static_cast<size_t>(row)),
                            stepperValues.at(static_cast<size_t>(row)));
            uiButton(rect, text.c_str(), false);
        }

        GuiSetState(STATE_NORMAL);
    }

    const int32_t hintY = MenuLayout::settingsMenuY +
                          (MenuLayout::settingsHeight + MenuLayout::settingsGap) * rowCount + 20;
    windowText(game, "Up/Down: select   Left/Right: change   Enter: confirm",
               game.resources.windowWidth / 2, hintY, 16, Palette::StructMid);
}

// Achievements: every one earned is a skill point (see progression.hpp). Two columns; earned
// rows bright with a tick, the rest dim with their progress where there is a count to show.
void drawAchievements(Game& game)
{
    applyGuiScale(game);
    const auto& ach = game.resources.achievements;
    const float scale = guiUiScale(game);

    constexpr int32_t columns = 2;
    constexpr int32_t rowHeight = 30;
    constexpr int32_t columnWidth = 470;
    constexpr int32_t rows = (achievementCount + columns - 1) / columns;
    constexpr float titleYRel = 20.0F;
    constexpr float summaryYRel = titleYRel + 50.0F;
    constexpr float gridTopYRel = summaryYRel + 45.0F;
    constexpr float contentHeightBase = gridTopYRel + static_cast<float>(rows * rowHeight) + 30.0F;

    const float viewportHeight = static_cast<float>(game.resources.windowHeight) - 150.0F * scale;
    const float contentHeight = contentHeightBase * scale;
    game.achievementsScroll.maxOffset = std::max(0.0F, contentHeight - viewportHeight);
    const float blockTop = std::max(0.0F, (viewportHeight - contentHeight) / 2) - game.achievementsScroll.offset;

    BeginScissorMode(0, 0, game.resources.windowWidth, static_cast<int32_t>(viewportHeight));
    windowText(game, "ACHIEVEMENTS", game.resources.windowWidth / 2,
               static_cast<int32_t>((blockTop + titleYRel * scale) / scale), 34, Palette::Accent);
    const auto earned = static_cast<int32_t>(std::ranges::count(ach.earned, true));
    const std::string summary =
        std::format("{} / {} earned   Skill points to spend: {}   Highest wave: {}", earned, achievementCount,
                    skillPoints(game), ach.highestWaveReached);
    windowText(game, summary.c_str(), game.resources.windowWidth / 2,
               static_cast<int32_t>((blockTop + summaryYRel * scale) / scale), 18, Palette::StructLight);

    const float leftX = static_cast<float>(game.resources.windowWidth) / 2 - static_cast<float>(columnWidth) * scale;
    const float topY = blockTop + gridTopYRel * scale;
    for (int32_t id = 0; id < achievementCount; id++)
    {
        const bool done = ach.earned.at(static_cast<size_t>(id));
        const float x = leftX + static_cast<float>(id / rows) * static_cast<float>(columnWidth) * scale;
        const float y = topY + static_cast<float>(id % rows) * static_cast<float>(rowHeight) * scale;
        std::string line = achievementName(id);
        if (!done && id < achievementWeaponMaxed)
        {
            line += std::format("  ({}/{})", ach.killsByWeapon.at(static_cast<size_t>(id)), evolutionKillThreshold);
        }
        else if (!done && id == achievementDashNerveKills)
        {
            line += std::format("  ({}/{})", ach.dashOrNerveKills, flakCannonUnlockKillCount);
        }
        const Color color = done ? Palette::Crit : Palette::StructMid;
        const Vector2 mark{.x = x + 10.0F * scale, .y = y + 10.0F * scale};
        if (done)
        {
            DrawLineEx(Vector2{.x = mark.x - 6 * scale, .y = mark.y}, Vector2{.x = mark.x - 2 * scale, .y = mark.y + 5 * scale},
                       2.0F * scale, color);
            DrawLineEx(Vector2{.x = mark.x - 2 * scale, .y = mark.y + 5 * scale}, Vector2{.x = mark.x + 6 * scale, .y = mark.y - 5 * scale},
                       2.0F * scale, color);
        }
        else
        {
            DrawCircleLines(static_cast<int32_t>(mark.x), static_cast<int32_t>(mark.y), 5.0F * scale, color);
        }
        drawText(game, line.c_str(), static_cast<int32_t>(x + 24.0F * scale), static_cast<int32_t>(y), 17, color);
    }
    EndScissorMode();

    const Rectangle back = backButtonRect(game);
    if (hoverActive(game) && CheckCollisionPointRec(GetMousePosition(), back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    uiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

// Skill tree: matte discs joined to their prerequisites. Owned nodes are lit, buyable ones pulse
// in the accent, the rest stay dark until a neighbour is bought.
void drawSkillTree(const Game& game)
{
    applyGuiScale(game);
    const float r = skillTreeNodeRadius(game);
    const float t = static_cast<float>(GetTime());

    const int32_t points = skillPoints(game);
    windowText(game, "SKILL TREE", game.resources.windowWidth / 2, 28, 34, Palette::Accent);
    const std::string header = game.sandbox ? std::string("Points: unlimited (sandbox)")
                                            : std::format("Points: {}", points);
    windowText(game, header.c_str(), game.resources.windowWidth / 2, 72, 20,
               points > 0 || game.sandbox ? Palette::Crit : Palette::StructLight);

    // Branch labels, beside the first node of each branch.
    const auto label = [&](const char* text, int32_t node, Vector2 off)
    {
        const Vector2 p = Vector2Add(skillTreeNodePos(game, node), Vector2Scale(off, r));
        const int32_t w = MeasureText(text, static_cast<int32_t>(r * 0.5F));
        DrawText(text, static_cast<int32_t>(p.x) - w / 2, static_cast<int32_t>(p.y), static_cast<int32_t>(r * 0.5F),
                 Palette::StructMid);
    };
    label("OFFENCE", 1, Vector2{.x = 0, .y = -2.2F});
    label("DEFENCE", 14, Vector2{.x = 0, .y = -2.2F});
    label("UTILITY", 25, Vector2{.x = 2.3F, .y = -0.25F});

    // Edges first, under the nodes.
    for (int32_t i = 0; i < skillNodeCount; i++)
    {
        for (const int8_t req : skillTree.at(static_cast<size_t>(i)).prereqs)
        {
            if (req < 0)
            {
                continue;
            }
            const Vector2 a = skillTreeNodePos(game, req);
            const Vector2 b = skillTreeNodePos(game, i);
            const Vector2 dir = Vector2Normalize(Vector2Subtract(b, a));
            const Vector2 from = Vector2Add(a, Vector2Scale(dir, r));
            const Vector2 to = Vector2Subtract(b, Vector2Scale(dir, r));
            const bool lit = skillNodeOwned(game, i);
            const bool open = skillNodeOwned(game, req);
            const Color color =
                lit ? Fade(Palette::Crit, 0.7F) : open ? Fade(Palette::Accent, 0.55F) : Palette::StructDark;
            const float len = Vector2Distance(from, to);
            if (len < r * 8.0F)
            {
                DrawLineEx(from, to, lit ? 3.0F : 2.0F, color);
                continue;
            }
            // A long link across branches is dotted, so it reads as a cross-link, not a path.
            for (float d = 0; d < len; d += r * 0.5F)
            {
                DrawCircleV(Vector2Add(from, Vector2Scale(dir, d)), lit ? 1.8F : 1.4F, color);
            }
        }
    }

    for (int32_t i = 0; i < skillNodeCount; i++)
    {
        const SkillNode& node = skillTree.at(static_cast<size_t>(i));
        const Vector2 c = skillTreeNodePos(game, i);
        const bool owned = skillNodeOwned(game, i);
        const bool available = skillNodeAvailable(game, i);
        const bool selected = i == game.skillTreeSelected;
        float s = popScale(game, i % 8);
        if (i == game.skillTreePopNode && game.skillTreePopTimer > 0)
        {
            s *= 1.0F + 0.35F * std::sin(game.skillTreePopTimer / 0.45F * PI);
        }
        if (s <= 0)
        {
            continue;
        }
        beginPop(c, s);
        const float pulse = 0.5F + 0.5F * std::sin(t * 4.0F + static_cast<float>(i));
        const Color ring = owned ? Palette::Crit : available ? Palette::Accent : Palette::StructMid;
        const Color icon = owned ? Palette::Crit : available ? Palette::Haze : Palette::StructMid;
        const float nodeR = node.kind == NodeKind::Ship ? r * 1.25F : r;
        if (available)
        {
            DrawCircleV(c, nodeR * (1.25F + 0.1F * pulse), Fade(Palette::Accent, 0.12F + 0.12F * pulse));
        }
        DrawCircleV(c, nodeR, owned ? Palette::StructDark : Palette::Void);
        DrawRing(c, nodeR - (selected ? 4.0F : 2.0F), nodeR, 0, 360, 48, ring);
        if (selected)
        {
            DrawRing(c, nodeR + 5.0F, nodeR + 7.0F, t * 90.0F, t * 90.0F + 300.0F, 40, Palette::Haze);
            if (game.skillTreeHold > 0) // unlock filling clockwise from the top
            {
                const float fill = std::min(game.skillTreeHold / skillTreeHoldDuration, 1.0F);
                DrawRing(c, nodeR + 4.0F, nodeR + 9.0F, -90.0F, -90.0F + 360.0F * fill, 64, Palette::Crit);
            }
        }
        const float iconS = nodeR * 0.48F;
        switch (node.kind)
        {
        case NodeKind::Ship:
            drawShipHull(static_cast<ShipClass>(node.value), c, nodeR * 0.55F, -PI / 2, icon);
            break;
        case NodeKind::Weapon:
            drawWeaponIcon(static_cast<WeaponType>(node.value), c, iconS, icon);
            break;
        case NodeKind::Evolution:
            drawWeaponIcon(static_cast<WeaponType>(node.value), c, iconS, icon, true);
            DrawRing(c, nodeR * 0.78F, nodeR * 0.82F, 0, 360, 40, Fade(icon, 0.6F)); // evolved: inner ring
            break;
        case NodeKind::Passive:
            drawPassiveIcon(static_cast<SkillType>(node.value), c, iconS, icon);
            break;
        case NodeKind::Slot:
        {
            const float h = iconS * 0.8F;
            DrawRectangleLinesEx(Rectangle{.x = c.x - h, .y = c.y - h, .width = h * 2, .height = h * 2}, 2.0F, icon);
            const std::string n = std::to_string(node.value);
            const auto fs = static_cast<int32_t>(h * 1.2F);
            DrawText(n.c_str(), static_cast<int32_t>(c.x) - MeasureText(n.c_str(), fs) / 2,
                     static_cast<int32_t>(c.y) - fs / 2, fs, icon);
            break;
        }
        }
        endPop();
    }

    // The selected node, described along the bottom.
    const int32_t sel = game.skillTreeSelected;
    const SkillNode& node = skillTree.at(static_cast<size_t>(sel));
    constexpr std::array<const char*, 5> kinds{"Ship (with its signature weapon and Nerve burst)", "Weapon",
                                               "Upgrade helper", "Evolution", "Weapon slot"};
    std::string state;
    if (skillNodeOwned(game, sel))
    {
        state = "Unlocked";
    }
    else if (skillNodeAvailable(game, sel))
    {
        state = game.sandbox || points > 0 ? "Hold to unlock for 1 point" : "Needs 1 point: earn achievements";
    }
    else
    {
        state = "Needs: ";
        bool first = true;
        for (const int8_t req : node.prereqs)
        {
            if (req >= 0 && !skillNodeOwned(game, req))
            {
                state += (first ? "" : ", ") + skillNodeName(req);
                first = false;
            }
        }
    }
    const float scale = guiUiScale(game);
    const Rectangle back = backButtonRect(game);
    const auto infoY = static_cast<int32_t>((back.y - 98.0F * scale) / scale);
    const std::string title = std::format("{}  -  {}", skillNodeName(sel), kinds.at(static_cast<size_t>(node.kind)));
    windowText(game, title.c_str(), game.resources.windowWidth / 2, infoY, 22, Palette::Haze);
    windowText(game, skillNodeDescription(sel).c_str(), game.resources.windowWidth / 2, infoY + 30, 17,
               Palette::StructLight);
    windowText(game, state.c_str(), game.resources.windowWidth / 2, infoY + 56, 17,
               skillNodeOwned(game, sel) ? Palette::Crit : Palette::Accent);

    if (hoverActive(game) && CheckCollisionPointRec(GetMousePosition(), back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    uiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

// Sandbox Weapon Gallery: every weapon's icon in its base and evolved colours, then the signature
// weapons' shots and the Nerve bursts drawn base and evolved side by side, with the game's own
// drawing functions. Laid out in 1920x1080 design units.
void drawWeaponGallery(const Game& game)
{
    applyGuiScale(game);
    const float scale = static_cast<float>(game.resources.windowHeight) / 1080.0F;
    const float offsetX = (static_cast<float>(game.resources.windowWidth) - 1920.0F * scale) / 2;
    BeginMode2D(Camera2D{.offset = {.x = offsetX, .y = 0}, .target = {}, .rotation = 0, .zoom = scale});
    const auto label = [&](const char* text, float x, float y, int32_t size, Color color, bool centred)
    {
        const int32_t w = MeasureText(text, size);
        DrawText(text, static_cast<int32_t>(x) - (centred ? w / 2 : 0), static_cast<int32_t>(y), size, color);
    };
    label("WEAPON GALLERY", 960, 30, 36, Palette::Accent, true);

    // All weapons: base icon, evolved icon (where it evolves), name.
    constexpr int32_t perRow = 8;
    for (int32_t i = 0; i < static_cast<int32_t>(WeaponType::Count); i++)
    {
        const auto type = static_cast<WeaponType>(i);
        const float cx = 180.0F + static_cast<float>(i % perRow) * 222.0F;
        const float cy = 120.0F + static_cast<float>(i / perRow) * 120.0F;
        const bool evolves = weaponEvolves.at(static_cast<size_t>(i));
        drawWeaponIcon(type, Vector2{.x = cx - (evolves ? 30.0F : 0.0F), .y = cy}, 16.0F, weaponColor(type));
        if (evolves)
        {
            drawWeaponIcon(type, Vector2{.x = cx + 30.0F, .y = cy}, 16.0F, evolvedWeaponColor(type), true);
        }
        label(std::string(weaponDisplayName(type)).c_str(), cx, cy + 28, 14, Palette::StructLight, true);
        if (evolves)
        {
            label(std::string(evolvedWeaponName.at(static_cast<size_t>(i))).c_str(), cx, cy + 46, 12,
                  evolvedWeaponColor(type), true);
        }
    }

    constexpr float baseX = 860.0F;
    constexpr float evolvedX = 1420.0F;
    float y = 400.0F;
    label("BASE", baseX, y - 40, 22, Palette::Haze, true);
    label("EVOLVED", evolvedX, y - 40, 22, Palette::Crit, true);
    const auto row = [&](const char* name, const auto& drawAt)
    {
        label(name, 160, y - 10, 20, Palette::StructLight, false);
        drawAt(Vector2{.x = baseX, .y = y}, false);
        drawAt(Vector2{.x = evolvedX, .y = y}, true);
        y += 92.0F;
    };
    const Vector2 flight = Vector2Normalize(Vector2{.x = 1, .y = -0.25F});
    row("Forward Shot (Ranger)", [&](Vector2 at, bool evolved)
        {
            for (int32_t k = -1; k <= 1; k++)
            {
                Bullet b{.position = Vector2Add(at, Vector2{.x = static_cast<float>(k) * 70.0F, .y = 0}),
                         .velocity = flight,
                         .radius = projectileSize,
                         .color = evolved ? evolvedWeaponColor(WeaponType::Forward) : weaponColor(WeaponType::Forward),
                         .active = true,
                         .damage = 0,
                         .evolved = evolved};
                drawPlayerBullet(b);
            }
        });
    const auto streaks = [&](Vector2 at, bool evolved, Color base, float lengthMult, float width)
    {
        for (int32_t k = 0; k < 5; k++)
        {
            Bullet b{.position = Vector2Add(at, Vector2{.x = static_cast<float>(k - 2) * 60.0F + 30.0F, .y = static_cast<float>((k * 7) % 3 - 1) * 14.0F}),
                     .velocity = flight,
                     .radius = width,
                     .color = evolved ? evolvedWeaponColor(WeaponType::LaserShots) : base,
                     .active = true,
                     .damage = 0,
                     .streak = laserShotStreak * lengthMult * (evolved ? laserEvoLengthMult : 1.0F),
                     .halfWidth = width,
                     .traveled = 1000.0F,
                     .evolved = evolved};
            drawPlayerBullet(b);
        }
    };
    row("Laser Shots (Interceptor)", [&](Vector2 at, bool evolved) { streaks(at, evolved, weaponColor(WeaponType::LaserShots), 1.0F, laserShotHalfWidth); });
    row("Orbit blades (Bastion)", [&](Vector2 at, bool evolved)
        {
            drawShipHull(ShipClass::Bastion, at, 16.0F, 0, Palette::StructMid);
            for (int32_t k = 0; k < 6; k++)
            {
                const float a = static_cast<float>(k) * PI / 3 + static_cast<float>(GetTime());
                drawOrbitRingBlade(Vector2Add(at, Vector2{.x = std::cos(a) * 38.0F, .y = std::sin(a) * 38.0F}),
                                   evolved ? evolvedWeaponColor(WeaponType::Orbit) : weaponColor(WeaponType::Orbit), evolved);
            }
        });
    row("Orbit shots (Bastion)", [&](Vector2 at, bool evolved)
        {
            for (int32_t k = -1; k <= 1; k++)
            {
                drawOrbitShot(Vector2Add(at, Vector2{.x = static_cast<float>(k) * 70.0F, .y = 0}), orbitProjectileRadius, evolved);
            }
        });
    row("Nerve: Ball (Ranger)", [&](Vector2 at, bool evolved) { drawNerveBall(at, 34.0F, evolved); });
    row("Nerve: Laser Storm (Interceptor)", [&](Vector2 at, bool evolved) { streaks(at, evolved, weaponColor(WeaponType::LaserShots), 1.8F, 2.0F); });
    row("Nerve: Blade Tornado (Bastion)", [&](Vector2 at, bool evolved)
        {
            for (int32_t k = 0; k < 6; k++)
            {
                const float a = static_cast<float>(k) * PI / 3 + static_cast<float>(GetTime()) * 3.0F;
                drawSpiralBlade(Vector2Add(at, Vector2{.x = std::cos(a) * 44.0F, .y = std::sin(a) * 44.0F}), evolved);
            }
        });
    EndMode2D();

    const Rectangle back = backButtonRect(game);
    uiButton(back, "Back");
}

// Predicts one Bestiary section's drawn height without drawing it, so
// updateBestiary can size its scroll range before drawBestiary runs. Keep
// this in sync with drawBestiarySection's own cellSize/gap/labelGap/columns
// math below.
auto bestiarySectionHeight(const Game& game, float scale, size_t count) -> float
{
    constexpr float cellSize = 52.0F;
    constexpr float gap = 14.0F;
    constexpr float labelGap = 20.0F;
    const float cellStep = (cellSize + gap) * scale;
    const float rowHeight = (cellSize + labelGap + gap) * scale;
    const float usableWidth = static_cast<float>(game.resources.windowWidth) * 0.88F;
    const auto columns = std::max(1, static_cast<int32_t>(usableWidth / cellStep));
    const auto rows = (static_cast<int32_t>(count) + columns - 1) / columns;
    return 34.0F * scale + static_cast<float>(rows) * rowHeight + 16.0F * scale;
}

auto bestiaryContentHeight(const Game& game, float scale) -> float
{
    constexpr float headerHeight = 140.0F; // "BESTIARY" title + summary line, unscaled
    return headerHeight * scale + bestiarySectionHeight(game, scale, enemyKinds.size()) +
          bestiarySectionHeight(game, scale, bestiaryBossKindCount) +
          bestiarySectionHeight(game, scale, bestiaryHazardKindCount);
}

namespace
{
auto drawBestiarySection(const Game& game, std::string_view title, float y, float scale,
                         size_t count, const std::function<bool(size_t)>& isUnlocked,
                         const std::function<std::string_view(size_t)>& nameFor,
                         const std::function<void(size_t, Vector2, float, Color)>& drawIcon,
                         const std::function<Color(size_t)>& colorFor) -> float
{
    constexpr float cellSize = 52.0F;
    constexpr float gap = 14.0F;
    constexpr float labelGap = 20.0F;
    const float cellStep = (cellSize + gap) * scale;
    const float rowHeight = (cellSize + labelGap + gap) * scale;

    const auto titleSize = static_cast<int32_t>(22 * scale);
    const std::string titleStr(title);
    const int32_t titleWidth = measureText(game, titleStr.c_str(), titleSize);
    drawText(game, titleStr.c_str(), game.resources.windowWidth / 2 - titleWidth / 2,
             static_cast<int32_t>(y), titleSize, Palette::Accent);
    y += 34 * scale;

    const float usableWidth = static_cast<float>(game.resources.windowWidth) * 0.88F;
    const auto columns = std::max(1, static_cast<int32_t>(usableWidth / cellStep));
    const float gridWidth = static_cast<float>(columns) * cellStep;
    const float leftX = (static_cast<float>(game.resources.windowWidth) - gridWidth) / 2;

    const Vector2 mouse = GetMousePosition();
    std::string hoverName;
    bool hovering = false;

    for (size_t i = 0; i < count; i++)
    {
        const auto column = static_cast<int32_t>(i) % columns;
        const auto row = static_cast<int32_t>(i) / columns;
        const float x = leftX + static_cast<float>(column) * cellStep;
        const float cellY = y + static_cast<float>(row) * rowHeight;
        const bool unlocked = isUnlocked(i);
        const Rectangle box{
            .x = x, .y = cellY, .width = cellSize * scale, .height = cellSize * scale};

        DrawRectangleLines(static_cast<int32_t>(box.x), static_cast<int32_t>(box.y),
                           static_cast<int32_t>(box.width), static_cast<int32_t>(box.height),
                           Fade(unlocked ? Palette::StructLight : Palette::StructMid, 0.6F));

        const Vector2 center{.x = box.x + box.width / 2, .y = box.y + box.height / 2};
        if (unlocked)
        {
            drawIcon(i, center, cellSize * scale * 0.36F, colorFor(i));
        }
        else
        {
            drawText(game, "?", static_cast<int32_t>(center.x - 7 * scale),
                     static_cast<int32_t>(center.y - 11 * scale), static_cast<int32_t>(22 * scale),
                     Palette::StructMid);
        }

        const std::string label = unlocked ? std::string(nameFor(i)) : std::string("???");
        const int32_t labelWidth = measureText(game, label.c_str(), 12);
        drawText(game, label.c_str(),
                 static_cast<int32_t>(x + box.width / 2 - static_cast<float>(labelWidth) / 2),
                 static_cast<int32_t>(cellY + box.height + 2 * scale), 12,
                 unlocked ? Palette::StructLight : Palette::StructMid);

        if (hoverActive(game) && CheckCollisionPointRec(mouse, box))
        {
            hovering = true;
            hoverName = unlocked ? std::string(nameFor(i)) : "??? (undiscovered)";
        }
    }

    const auto rows = (static_cast<int32_t>(count) + columns - 1) / columns;
    const float sectionEnd = y + static_cast<float>(rows) * rowHeight;

    if (hovering)
    {
        const auto hoverSize = static_cast<int32_t>(16 * scale);
        const int32_t hoverWidth = measureText(game, hoverName.c_str(), hoverSize);
        drawText(game, hoverName.c_str(), game.resources.windowWidth / 2 - hoverWidth / 2,
                 static_cast<int32_t>(sectionEnd), hoverSize, Palette::Crit);
        return sectionEnd + 30 * scale;
    }

    return sectionEnd + 16 * scale;
}
}

void drawBestiary(Game& game)
{
    applyGuiScale(game);
    const auto& bestiary = game.resources.bestiary;
    const float scale = guiUiScale(game);

    // Whole screen (title through footer) is one invisible container,
    // centered in the space above the Back button - and, since the section
    // grids' height depends on window width, scrollable if it doesn't fit.
    const float viewportHeight = static_cast<float>(game.resources.windowHeight) - 150.0F * scale;
    const float contentHeight = bestiaryContentHeight(game, scale);
    game.bestiaryScroll.maxOffset = std::max(0.0F, contentHeight - viewportHeight);
    const float blockTop =
        std::max(0.0F, (viewportHeight - contentHeight) / 2) - game.bestiaryScroll.offset;

    BeginScissorMode(0, 0, game.resources.windowWidth, static_cast<int32_t>(viewportHeight));

    windowText(game, "BESTIARY", game.resources.windowWidth / 2,
              static_cast<int32_t>((blockTop + 60.0F * scale) / scale), 34, Palette::Accent);

    const auto enemyCount =
        std::count(bestiary.enemyKilled.begin(), bestiary.enemyKilled.end(), true);
    const auto bossCount = std::count(bestiary.bossKilled.begin(), bestiary.bossKilled.end(), true);
    const auto hazardCount =
        std::count(bestiary.hazardKilled.begin(), bestiary.hazardKilled.end(), true);
    const std::string summary = std::format(
        "Enemies: {} of {}   Bosses: {} of {}   Hazards: {} of {}", enemyCount, enemyKinds.size(),
        bossCount, bestiaryBossKindCount, hazardCount, bestiaryHazardKindCount);
    windowText(game, summary.c_str(), game.resources.windowWidth / 2,
              static_cast<int32_t>((blockTop + 100.0F * scale) / scale), 18, Palette::StructLight);

    float y = blockTop + 140.0F * scale;
    y = drawBestiarySection(
        game, "ENEMIES", y, scale, enemyKinds.size(),
        [&](size_t i) { return bestiary.enemyKilled.at(i); }, [](size_t i) -> std::string_view
        { return enemyKinds.at(i).name; }, [](size_t i, Vector2 c, float r, Color col)
        {
            const EnemyShape shape = enemyKinds.at(i).shape;
            if (shape == EnemyShape::Organic)
            {
                drawOrganicEnemyShape(c, r, static_cast<float>(i), col);
            }
            else if (shape == EnemyShape::Cluster)
            {
                drawClusterEnemyShape(c, r, static_cast<float>(i), col);
            }
            else
            {
                drawEnemyShape(shape, c, r, -90, col, 1);
            }
        },
        [](size_t i) { return enemyKinds.at(i).color; });

    y = drawBestiarySection(
        game, "BOSSES", y, scale, bestiaryBossKindCount,
        [&](size_t i) { return bestiary.bossKilled.at(i); },
        [](size_t i) { return bossBestiaryName(static_cast<int32_t>(i)); },
        [](size_t i, Vector2 c, float r, Color col)
        {
            drawBossHull(bossBestiaryShape(static_cast<int32_t>(i)), c,
                         Vector2{.x = r * 2, .y = r * 2}, col);
        },
        [](size_t i) { return bossBestiaryColor(static_cast<int32_t>(i)); });

    y = drawBestiarySection(
        game, "HAZARDS", y, scale, bestiaryHazardKindCount,
        [&](size_t i) { return bestiary.hazardKilled.at(i); },
        [](size_t i) { return hazardBestiaryName(static_cast<EliteHazardRole>(i)); },
        [](size_t /*i*/, Vector2 c, float r, Color col)
        {
            DrawCircleV(c, r, Palette::StructDark);
            DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), r, col);
            DrawPolyLines(c, 6, r * 1.3F, 0, Fade(col, 0.7F));
        },
        [](size_t i)
        {
            return static_cast<EliteHazardRole>(i) == EliteHazardRole::Warlord ? Palette::Accent
                                                                               : Palette::Shield;
        });

    const auto footerSize = static_cast<int32_t>(16 * scale);
    const char* footerText =
        touchInputActive(game) ? "Tap & hold an entry for its name" : "Hover an entry for its name";
    const int32_t footerWidth = measureText(game, footerText, footerSize);
    drawText(game, footerText, game.resources.windowWidth / 2 - footerWidth / 2,
             static_cast<int32_t>(y), footerSize, Palette::StructMid);

    EndScissorMode();

    const Vector2 mouse = GetMousePosition();
    const Rectangle back = backButtonRect(game);
    if (hoverActive(game) && CheckCollisionPointRec(mouse, back))
    {
        GuiSetState(STATE_FOCUSED);
    }
    uiButton(back, "Back");
    GuiSetState(STATE_NORMAL);
}

void drawSkillIcon(SkillType id, Vector2 c, float s, Color color, bool evolved)
{
    if (const auto kind = weaponForGrantSkill(id); kind.has_value())
    {
        drawWeaponIcon(*kind, c, s, color, evolved);
        return;
    }
    drawPassiveIcon(id, c, s, color);
}

void drawWeaponIcon(WeaponType kind, Vector2 c, float s, Color color, bool evolved)
{
    switch (kind)
    {
    case WeaponType::Forward:
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s},
                     Vector2{.x = c.x - s * 0.7F, .y = c.y + s * 0.7F},
                     Vector2{.x = c.x + s * 0.7F, .y = c.y + s * 0.7F}, color);
        break;
    case WeaponType::Orbit:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleV(Vector2{.x = c.x + s, .y = c.y}, s * 0.25F, color);
        break;
    case WeaponType::Homing:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleV(c, s * 0.3F, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y - s}, Vector2{.x = c.x, .y = c.y - s * 1.7F}, 2,
                   color);
        break;
    case WeaponType::Mine:
        DrawCircleV(c, s * 0.55F, color);
        for (int a = 0; a < 360; a += 60)
        {
            const float rad = static_cast<float>(a) * DEG2RAD;
            const Vector2 tip{.x = c.x + std::cos(rad) * s, .y = c.y + std::sin(rad) * s};
            DrawLineEx(c, tip, 2, color);
        }
        break;
    case WeaponType::Beam:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s, .y = c.y}, 3, color);
        DrawCircleV(Vector2{.x = c.x - s, .y = c.y}, s * 0.25F, color);
        break;
    case WeaponType::Shock:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.5F, color);
        break;
    case WeaponType::Ricochet:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y - s}, Vector2{.x = c.x, .y = c.y + s}, 2, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y + s}, Vector2{.x = c.x + s, .y = c.y - s}, 2, color);
        break;
    case WeaponType::FollowerDrone:
        DrawCircleV(c, s * 0.4F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        break;
    case WeaponType::LaserDrone:
        DrawCircleV(c, s * 0.4F, color);
        DrawLineEx(c, Vector2{.x = c.x + s, .y = c.y}, 2, color);
        break;
    case WeaponType::FlakCannon:
        DrawCircleV(c, s * 0.5F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, Fade(color, 0.5F));
        break;
    case WeaponType::Railgun:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s, .y = c.y}, 4, color);
        break;
    case WeaponType::ChainLightning:
        DrawLineEx(Vector2{.x = c.x - s, .y = c.y - s}, Vector2{.x = c.x, .y = c.y}, 2, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y}, Vector2{.x = c.x + s, .y = c.y - s}, 2, color);
        break;
    case WeaponType::TurretDeploy:
        DrawRectangle(static_cast<int32_t>(c.x - s * 0.4F), static_cast<int32_t>(c.y - s * 0.4F),
                      static_cast<int32_t>(s * 0.8F), static_cast<int32_t>(s * 0.8F), color);
        break;
    case WeaponType::Flamethrower:
        DrawTriangle(Vector2{.x = c.x - s, .y = c.y}, Vector2{.x = c.x + s * 0.6F, .y = c.y + s},
                     Vector2{.x = c.x + s * 0.6F, .y = c.y - s}, color);
        break;
    case WeaponType::LaserShots:
        // Evolved: the same streaks, heavier.
        for (const float dy : {-0.5F, 0.0F, 0.5F})
        {
            DrawLineEx(Vector2{.x = c.x - s + (dy == 0 ? s * 0.4F : 0), .y = c.y + dy * s},
                       Vector2{.x = c.x + s * (dy == 0 ? 1.0F : 0.6F), .y = c.y + dy * s}, evolved ? 2.5F : 1.5F,
                       color);
        }
        break;
    case WeaponType::Sniper:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawLineEx(Vector2{.x = c.x - s * 1.3F, .y = c.y}, Vector2{.x = c.x + s * 1.3F, .y = c.y},
                   1.5F, color);
        DrawLineEx(Vector2{.x = c.x, .y = c.y - s * 1.3F}, Vector2{.x = c.x, .y = c.y + s * 1.3F},
                   1.5F, color);
        break;
    case WeaponType::Count:
        break;
    }
}

void drawPassiveIcon(SkillType id, Vector2 c, float s, Color color)
{
    switch (id)
    {
    case SkillType::Damage:
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s}, Vector2{.x = c.x - s * 0.6F, .y = c.y + s},
                     Vector2{.x = c.x + s * 0.6F, .y = c.y + s}, color);
        break;
    case SkillType::Barrier:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.55F, color);
        break;
    case SkillType::Cooldown:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, color);
        DrawLineEx(c, Vector2{.x = c.x, .y = c.y - s * 0.8F}, 2, color);
        DrawLineEx(c, Vector2{.x = c.x + s * 0.5F, .y = c.y}, 2, color);
        break;
    case SkillType::PickupRadius:
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s * 0.5F, color);
        DrawCircleLines(static_cast<int32_t>(c.x), static_cast<int32_t>(c.y), s, Fade(color, 0.5F));
        break;
    case SkillType::MoveSpeed:
        DrawTriangle(Vector2{.x = c.x - s, .y = c.y - s * 0.6F},
                     Vector2{.x = c.x - s, .y = c.y + s * 0.6F}, Vector2{.x = c.x, .y = c.y},
                     color);
        DrawTriangle(Vector2{.x = c.x, .y = c.y - s * 0.6F}, Vector2{.x = c.x, .y = c.y + s * 0.6F},
                     Vector2{.x = c.x + s, .y = c.y}, color);
        break;
    case SkillType::MaxHp:
        DrawRectangle(static_cast<int32_t>(c.x - s * 0.15F), static_cast<int32_t>(c.y - s),
                      static_cast<int32_t>(s * 0.3F), static_cast<int32_t>(s * 2), color);
        DrawRectangle(static_cast<int32_t>(c.x - s), static_cast<int32_t>(c.y - s * 0.15F),
                      static_cast<int32_t>(s * 2), static_cast<int32_t>(s * 0.3F), color);
        break;
    default:
        break;
    }
}

auto elementEffectSummary(ElementType element) -> std::string_view
{
    switch (element)
    {
    case ElementType::Static:
        return "fully stops movement, attacks, and contact damage";
    case ElementType::Freeze:
        return "halves movement and attack speed";
    case ElementType::Burn:
        return "deals damage over time";
    case ElementType::Count:
        break;
    }
    return "";
}

auto pickupChoiceDescription(PickupType type, ElementType element, ElementMechanism mechanism)
    -> std::string
{
    switch (type)
    {
    case PickupType::LifeOrb:
        return "Instantly heals a small amount of health.";
    case PickupType::Shield:
        return "+1 shield stack (blocks one hit).";
    case PickupType::Regen:
        return "Heals gradually for a while.";
    case PickupType::MagnetPulse:
        return "Pulls every pickup on the map to you.";
    case PickupType::Overcharge:
        return "+50% weapon damage for a while.";
    case PickupType::SecondWind:
        return "Permanent: survive the next otherwise-fatal hit.";
    case PickupType::Overdrive:
        return "Dash and shield-block cost no charge, for a while.";
    case PickupType::Elemental:
    {
        const auto effect = elementEffectSummary(element);
        switch (mechanism)
        {
        case ElementMechanism::Infusion:
            return std::format("Your hits apply {} to enemies for a while ({}).",
                               elementNames.at(static_cast<size_t>(element)), effect);
        case ElementMechanism::Nova:
            return std::format("Instantly applies {} to every enemy on screen ({}).",
                               elementNames.at(static_cast<size_t>(element)), effect);
        case ElementMechanism::Count:
            break;
        }
        return "";
    }
    case PickupType::XP:
    case PickupType::Danger:
    case PickupType::Count:
        break;
    }
    return "Applied instantly.";
}

auto evolutionHint(const Game& game, SkillType id) -> std::string
{

    for (size_t i = 0; i < weaponEvolves.size(); i++)
    {
        const auto kind = static_cast<WeaponType>(i);
        if (weaponEvolves.at(i) && skillLinkedPassive.at(i) == id && hasWeapon(game, kind))
        {
            return std::string(evolvedWeaponName.at(i));
        }
    }
    return "";
}

void drawAbilitySlots(const Game& game, int32_t x, int32_t y)
{
    constexpr float boxSize = 28;
    constexpr float gap = 34;
    constexpr float iconScale = 0.4F;

    struct Slot
    {
        SkillType id;
        Color color;
        int32_t level = 0;
        bool stolen = false;
        bool evolutionReady = false;
        bool evolved = false;
    };
    std::vector<Slot> slots;

    for (const auto& w : game.run.weapons)
    {
        const Color color = w.evolved ? evolvedWeaponColor(w.type) : weaponColor(w.type);
        const auto typeIndex = static_cast<size_t>(w.type);
        const bool evolutionReady = weaponEvolves.at(typeIndex) && !w.evolved && isEvolutionUnlocked(game, w.type);
        const bool stolen =
            game.run.weaponDowngrade.has_value() && game.run.weaponDowngrade->type == w.type;
        slots.push_back(Slot{.id = weaponGrantSkill.at(typeIndex),
                             .color = color,
                             .level = w.level,
                             .stolen = stolen,
                             .evolutionReady = evolutionReady,
                             .evolved = w.evolved});
    }

    for (size_t i = 0; i < static_cast<size_t>(SkillType::Count); i++)
    {
        const auto id = static_cast<SkillType>(i);
        if (game.run.skillLevels.at(i) == 0 || isFusedPassive(game, id))
        {
            continue;
        }
        if (weaponForGrantSkill(id).has_value())
        {
            continue;
        }
        slots.push_back(
            Slot{.id = id, .color = Palette::Shield, .level = game.run.skillLevels.at(i)});
    }

    const Vector2 mouse = mouseUIPos(game);
    std::optional<SkillType> hovered;
    Vector2 hoveredBoxPos{};

    // Bare icons, level dots under each, no boxes; an empty slot is a faint dot.
    for (int32_t i = 0; i < weaponSlotCap(game); i++)
    {
        const float bx = static_cast<float>(x) + static_cast<float>(i) * gap;
        const auto by = static_cast<float>(y);
        const Vector2 center{.x = bx + boxSize / 2, .y = by + boxSize / 2};
        if (static_cast<size_t>(i) >= slots.size())
        {
            DrawCircleLinesV(center, 2.5F, Fade(Palette::StructMid, 0.6F));
            continue;
        }
        const Slot& slot = slots.at(static_cast<size_t>(i));
        const Color color = slot.stolen ? Palette::Accent : slot.color;
        if (slot.evolved)
        {
            DrawCircleV(center, boxSize * 0.5F, Fade(Palette::Crit, 0.08F));
        }
        drawSkillIcon(slot.id, center, boxSize * iconScale, color, slot.evolved);
        for (int32_t l = 0; l < slot.level; l++) // level dots, two rows of four past level 4
        {
            const float dx = center.x - 7.5F + static_cast<float>(l % 4) * 5.0F;
            const float dy = by + boxSize + 4 + static_cast<float>(l / 4) * 4.0F;
            DrawRectangleRec(Rectangle{.x = dx - 1, .y = dy - 1, .width = 2, .height = 2}, Fade(color, 0.8F));
        }
        if (slot.evolutionReady)
        {
            DrawTriangle(Vector2{.x = center.x - 3, .y = by - 2}, Vector2{.x = center.x + 3, .y = by - 2},
                         Vector2{.x = center.x, .y = by - 6}, Palette::Heal);
        }
        if (hoverActive(game) &&
            CheckCollisionPointRec(mouse, Rectangle{.x = bx, .y = by, .width = boxSize, .height = boxSize}))
        {
            hovered = slot.id;
            hoveredBoxPos = Vector2{.x = bx, .y = by};
        }
    }

    if (hovered.has_value())
    {
        const auto& def = Skills.at(static_cast<size_t>(*hovered));
        const int lvl = game.run.skillLevels.at(static_cast<size_t>(*hovered));
        std::string_view skillName = def.name;
        if (const auto weapon = weaponForGrantSkill(*hovered);
            weapon.has_value() && weaponEvolved(game, *weapon))
        {
            skillName = evolvedWeaponName.at(static_cast<size_t>(*weapon));
        }
        const std::string name = std::format("{} (Lv {})", skillName, lvl);
        const std::string desc(def.description);

        constexpr int32_t nameFontSize = 16;
        constexpr int32_t descFontSize = 13;
        const int32_t boxWidth = std::max(measureText(game, name.c_str(), nameFontSize),
                                          measureText(game, desc.c_str(), descFontSize)) +
                                 16;
        const auto scaledScreenWidth =
            static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
        auto tipX = static_cast<int32_t>(hoveredBoxPos.x);
        if (tipX + boxWidth > scaledScreenWidth)
        {
            tipX = scaledScreenWidth - boxWidth;
        }
        const int32_t tipY = static_cast<int32_t>(hoveredBoxPos.y) - 52; // above: the slots sit at the bottom

        DrawRectangle(tipX, tipY, boxWidth, 46, Fade(Palette::Void, 0.85F));
        DrawRectangle(tipX, tipY, 2, 46, Palette::Accent);
        drawText(game, name.c_str(), tipX + 8, tipY + 6, nameFontSize, Palette::Accent);
        drawText(game, desc.c_str(), tipX + 8, tipY + 26, descFontSize, Palette::StructLight);
    }
}

void drawLevelUp(const Game& game)
{
    applyGuiScale(game);
    const float scale = guiUiScale(game);
    beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2, .y = 117.0F * scale}, popScale(game, 0));
    windowText(game, std::format("LEVEL {}", game.run.level).c_str(),
               game.resources.windowWidth / 2, 100, 34, Palette::Accent);
    endPop();

    const auto count = static_cast<int32_t>(game.run.pendingChoices.size());
    constexpr int32_t nameFontSize = 24;
    constexpr int32_t descFontSize = 15;
    constexpr int32_t iconOffsetX = 24;
    constexpr float iconRadius = 11;

    for (int32_t i = 0; i < count; i++)
    {
        const auto& choice = game.run.pendingChoices.at(static_cast<size_t>(i));
        const Rectangle rect =
            menuColumnRect(game, i, count, MenuLayout::levelUpWidth, MenuLayout::levelUpHeight,
                           MenuLayout::levelUpGap, MenuLayout::levelUpMenuY);

        Color color = Palette::StructLight;
        std::string name;
        std::string desc;
        SkillType icon{};
        bool hasIcon = false;
        bool evolvedIcon = false;

        switch (choice.type)
        {
        case ChoiceType::Evolve:
            name = std::format("EVOLVE: {}",
                               evolvedWeaponName.at(static_cast<size_t>(choice.weapon.value())));
            desc = "Fuses the weapon and its linked passive into a super weapon.";
            icon = weaponGrantSkill.at(static_cast<size_t>(choice.weapon.value()));
            hasIcon = true;
            evolvedIcon = true;
            if (i == game.menuIndex)
            {
                color = Palette::Crit;
            }
            break;
        case ChoiceType::Pickup:
        {

            const auto found = std::ranges::find_if(pickupCatalog,
                                                    [&](const PickupCatalogEntry& e)
                                                    {
                                                        return e.type == choice.pickupType &&
                                                               e.element == choice.element &&
                                                               e.mechanism == choice.mechanism;
                                                    });
            name = found != pickupCatalog.end() ? std::string(found->name) : "Pickup";
            desc = pickupChoiceDescription(choice.pickupType, choice.element, choice.mechanism);
            if (choice.pickupType == PickupType::Elemental)
            {
                color = elementColors.at(static_cast<size_t>(choice.element));
            }
            break;
        }
        case ChoiceType::Ascension:
        {
            const auto& def = ascensions.at(static_cast<size_t>(choice.ascension));
            const int stacks = game.run.ascensionStacks.at(static_cast<size_t>(choice.ascension));
            name = stacks > 0 ? std::format("{} (x{})", def.name, stacks + 1) : std::string(def.name);
            desc = def.description;
            color = Palette::Charge;
            break;
        }
        case ChoiceType::Skill:
        default:
        {
            const auto& def = Skills.at(static_cast<size_t>(choice.skill));
            const int lvl = game.run.skillLevels.at(static_cast<size_t>(choice.skill));
            std::string_view skillName = def.name;
            if (const auto weapon = weaponForGrantSkill(choice.skill);
                weapon.has_value() && weaponEvolved(game, *weapon))
            {
                skillName = evolvedWeaponName.at(static_cast<size_t>(*weapon));
            }
            name = std::format("{} (Lv {})", skillName, lvl + 1);
            desc = def.description;
            if (const auto hint = evolutionHint(game, choice.skill); !hint.empty())
            {
                desc = std::format("{}  ->  builds toward {}", desc, hint);
            }
            icon = choice.skill;
            hasIcon = true;
            break;
        }
        }

        if (i == game.menuIndex)
        {
            if (choice.type == ChoiceType::Evolve)
            {
                color = Palette::Crit;
            }
            else if (choice.type == ChoiceType::Pickup &&
                     choice.pickupType == PickupType::Elemental)
            {
                color = elementColors.at(static_cast<size_t>(choice.element));
            }
            else
            {
                color = Palette::Accent;
            }
        }

        beginPop(Vector2{.x = rect.x + rect.width / 2, .y = rect.y + rect.height / 2}, popScale(game, i + 1));
        const auto nameSize = static_cast<int32_t>(static_cast<float>(nameFontSize) * scale);
        const auto descSize = static_cast<int32_t>(static_cast<float>(descFontSize) * scale);
        const int32_t nameY = static_cast<int32_t>(rect.y + rect.height * 0.28F) - nameSize / 2;
        const int32_t descY = static_cast<int32_t>(rect.y + rect.height * 0.68F) - descSize / 2;
        const int32_t nameX = static_cast<int32_t>(rect.x + rect.width / 2) -
                              measureText(game, name.c_str(), nameSize) / 2;

        if (hasIcon)
        {
            drawSkillIcon(
                icon,
                Vector2{.x = static_cast<float>(nameX) - static_cast<float>(iconOffsetX) * scale,
                        .y = static_cast<float>(nameY) + static_cast<float>(nameSize) / 2},
                iconRadius * scale, color, evolvedIcon);
        }
        drawText(game, name.c_str(), nameX, nameY, nameSize, color);
        windowText(game, desc.c_str(), static_cast<int32_t>(rect.x + rect.width / 2),
                   static_cast<int32_t>(static_cast<float>(descY) / scale), descFontSize,
                   Palette::StructMid);
        endPop();
    }
}

void drawMenu(const Game& game, const std::string& heading, const std::vector<std::string>& options,
              int32_t y)
{
    applyGuiScale(game);
    if (!heading.empty())
    {
        beginPop(Vector2{.x = static_cast<float>(game.resources.windowWidth) / 2,
                         .y = static_cast<float>(y - 35) * guiUiScale(game)},
                 popScale(game, 0));
        windowText(game, heading.c_str(), game.resources.windowWidth / 2, y - 50, 30,
                   Palette::StructLight);
        endPop();
    }

    const auto count = static_cast<int32_t>(options.size());
    for (int32_t i = 0; i < count; i++)
    {
        const Rectangle rect = menuColumnRect(game, i, count, MenuLayout::buttonWidth,
                                              MenuLayout::buttonHeight, MenuLayout::buttonGap, y);
        if (i == game.menuIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }
        beginPop(Vector2{.x = rect.x + rect.width / 2, .y = rect.y + rect.height / 2}, popScale(game, i + 1));
        uiButton(rect, options.at(static_cast<size_t>(i)).c_str(), false);
        endPop();
        GuiSetState(STATE_NORMAL);
    }
}

// x/y are unscaled (like windowText) - scaling happens internally, so the
// block's own height always stays proportional to the gap callers reserve
// for it, at any guiUiScale.
void drawHighScores(const Game& game, int32_t centerX, int32_t y, const std::vector<int32_t>& scores)
{
    windowText(game, "Top Scores", centerX, y, 26, Palette::AccentDim);

    if (scores.empty())
    {
        windowText(game, "(none yet)", centerX, y + 36, 22, Palette::StructMid);
        return;
    }

    for (size_t i = 0; i < scores.size(); i++)
    {
        const std::string line = std::format("{}. {}", i + 1, withThousands(scores.at(i)));
        windowText(game, line.c_str(), centerX, y + 36 + static_cast<int32_t>(i) * 28, 22,
                  Palette::StructLight);
    }
}

void drawBrowserParticleDemo(int32_t style, Vector2 center)
{
    const auto t = static_cast<float>(GetTime());
    switch (style)
    {
    case 0:
        for (int32_t i = 0; i < 10; i++)
        {
            const float seed = static_cast<float>(i) * 17.0F;
            const float angle = std::fmod(seed + t * 1.5F, 2.0F * std::numbers::pi_v<float>);
            const float bob = 0.5F + 0.5F * std::sin(t * 9.0F + seed);
            const Vector2 base{.x = center.x + std::cos(angle) * 40,
                               .y = center.y + std::sin(angle) * 40 * 0.5F + 40};
            const Vector2 tip{.x = base.x + std::sin(t * 14.0F + seed) * 4,
                              .y = base.y - (12.0F + bob * 20.0F)};
            const Color flameColor =
                bob > 0.6F ? Palette::ElementBurn : ColorLerp(Palette::ElementBurn, YELLOW, 0.5F);
            DrawTriangle(Vector2{.x = base.x - 4, .y = base.y}, tip,
                         Vector2{.x = base.x + 4, .y = base.y}, Fade(flameColor, 0.75F));
        }
        break;
    case 1:
    default:
        for (int32_t i = 0; i < 16; i++)
        {
            const float angle = static_cast<float>(i) * (360.0F / 16.0F) * DEG2RAD;
            const float life = std::fmod(t * 1.2F + static_cast<float>(i) * 0.05F, 1.0F);
            const Vector2 pos{.x = center.x + std::cos(angle) * life * 70,
                              .y = center.y + std::sin(angle) * life * 70};
            const Color debrisColor = i % 2 == 0 ? Palette::Accent : Palette::Crit;
            DrawCircleV(pos, 4 * (1 - life), Fade(debrisColor, 1 - life));
        }
        break;
    }
}

void drawSandboxBrowser(const Game& game)
{
    for (int32_t i = 0; i < browserCategoryCount; i++)
    {
        const Rectangle rect = sandboxBrowserCategoryButtonRect(game, i);
        if (i == game.browserCategoryIndex)
        {
            GuiSetState(STATE_FOCUSED);
        }
        uiButton(rect, std::string(browserCategoryLabel(i)).c_str());
        GuiSetState(STATE_NORMAL);
    }

    const Rectangle preview = sandboxBrowserPreviewRect(game);
    DrawRectangleRec(preview, Fade(Palette::StructDark, 0.6F));
    DrawRectangleLinesEx(preview, 2, Palette::StructLight);
    const Vector2 center{.x = preview.x + preview.width / 2, .y = preview.y + preview.height / 2};

    switch (game.browserCategoryIndex)
    {
    case 0:
    {
        const auto& kind = enemyKinds.at(static_cast<size_t>(game.sandboxKindIndex));
        Enemy previewEnemy{.kind = game.sandboxKindIndex,
                           .position = center,
                           .velocity = Vector2{},
                           .health = kind.health,
                           .active = true,
                           .stateTimer = 1,
                           .charging = false,
                           .telegraphing = false,
                           .phased = false,
                           .orbitAngle = 0,
                           .orbitDist = 0,
                           .orbitCenterCurrent = center,
                           .isElite = false,
                           .hitByDash = false,
                           .hitFlashTimer = 0};
        drawEnemy(game, previewEnemy, false);
        break;
    }
    case 1:
    {
        const auto& type = bossTypes.at(static_cast<size_t>(game.browserBossTypeIndex));
        constexpr float previewBossSize = 110.0F;
        Boss previewBoss{.position = Vector2{.x = center.x - previewBossSize / 2,
                                             .y = center.y - previewBossSize / 2},
                         .size = Vector2{.x = previewBossSize, .y = previewBossSize},
                         .color = type.color,
                         .baseColor = type.color,
                         .health = 100,
                         .maxHealth = 100,
                         .state = BossState::IDLE,
                         .shape = type.shape};
        drawBoss(game, previewBoss);
        break;
    }
    case 2:
    {
        const auto shipClass = static_cast<ShipClass>(game.browserShipIndex);
        drawShipHull(shipClass, center, 20, 0,
                     ships.at(static_cast<size_t>(game.browserShipIndex)).color);
        break;
    }
    case 3:
    {
        const auto option = sandboxPickupPreview(game.sandboxPickupIndex);
        drawPickupIcon(option.type, option.element, center, 1.0F);
        break;
    }
    case 4:
        drawWeaponIcon(static_cast<WeaponType>(game.browserWeaponIndex), center, 26,
                       Palette::Accent);
        break;
    case 5:
        drawBrowserParticleDemo(game.browserParticleIndex, center);
        break;
    default:
        break;
    }

    windowText(game, browserCurrentName(game).c_str(), static_cast<int32_t>(center.x),
               static_cast<int32_t>(preview.y + preview.height + 30), 22, Palette::Crit);

    if (const std::string details = browserCurrentDetails(game); !details.empty())
    {
        windowText(game, details.c_str(), static_cast<int32_t>(center.x),
                   static_cast<int32_t>(preview.y + preview.height + 62), 16, Palette::StructLight);
    }

    uiButton(sandboxBrowserPrevButtonRect(game), "<");
    uiButton(sandboxBrowserNextButtonRect(game), ">");
}

void drawPickupIcon(PickupType type, ElementType element, Vector2 position, float alpha)
{
    switch (type)
    {
    case PickupType::LifeOrb:
        DrawCircleV(position, 5, Fade(Palette::Accent, alpha));
        DrawCircleLinesV(position, 6, Fade(Palette::Crit, 0.8F * alpha));
        break;
    case PickupType::Shield:
        DrawCircleV(position, 6, Fade(Palette::Shield, alpha));
        DrawCircleLinesV(position, 7, Fade(Palette::Haze, 0.8F * alpha));
        break;
    case PickupType::Elemental:
    {
        const Color color = elementColors.at(static_cast<size_t>(element));
        DrawPoly(position, 4, 7, static_cast<float>(GetTime()) * 90.0F, Fade(color, alpha));
        DrawCircleLinesV(position, 9, Fade(color, 0.6F * alpha));
        break;
    }
    case PickupType::Regen:
        DrawCircleV(position, 6, Fade(Palette::Heal, alpha));
        DrawCircleLinesV(position, 7, Fade(WHITE, 0.6F * alpha));
        break;
    case PickupType::MagnetPulse:
        DrawCircleLinesV(position, 8, Fade(Palette::Shield, alpha));
        DrawCircleLinesV(position, 5, Fade(Palette::Shield, 0.6F * alpha));
        break;
    case PickupType::Overcharge:
        DrawPoly(position, 6, 7, static_cast<float>(GetTime()) * 120.0F,
                 Fade(Palette::Charge, alpha));
        break;
    case PickupType::SecondWind:
        DrawCircleV(position, 6, Fade(Palette::Crit, alpha));
        DrawCircleLinesV(position, 8, Fade(Palette::Crit, 0.5F * alpha));
        break;
    case PickupType::Danger:
    {
        const float pulse = 0.5F + 0.5F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        DrawPoly(position, 5, 12, static_cast<float>(GetTime()) * -140.0F,
                 Fade(Palette::Accent, alpha));
        DrawPolyLines(position, 5, 13, static_cast<float>(GetTime()) * -140.0F,
                      Fade(Palette::Crit, alpha * (0.5F + 0.5F * pulse)));
        DrawRectangleV(Vector2{.x = position.x - 1.5F, .y = position.y - 6.0F}, Vector2{.x = 3.0F, .y = 7.0F},
                       Fade(Palette::Crit, alpha));
        DrawRectangleV(Vector2{.x = position.x - 1.5F, .y = position.y + 3.0F}, Vector2{.x = 3.0F, .y = 3.0F},
                       Fade(Palette::Crit, alpha));
        break;
    }
    default:
        DrawCircleV(position, 2.5F, Fade(Palette::Charge, alpha));
        DrawCircleLinesV(position, 3.5F, Fade(Palette::Crit, 0.6F * alpha));
        break;
    }
}

void drawSandboxToggleIndicator(const Game& game)
{
    applyGuiScale(game);
    const Rectangle rect = sandboxToggleIndicatorRect(game);
    uiButton(rect, "Sandbox [TAB]");

    // M12 director debug panel: what the bot/player is doing and how each boss is responding.
    const float scale = guiUiScale(game);
    const auto px = [scale](float v) { return static_cast<int32_t>(v * scale); };
    const int32_t font = px(20);
    const int32_t line = px(24);
    const int32_t barW = px(140);
    const int32_t labelW = px(125);
    const int32_t panelW = labelW + barW + px(170);
    // Right side, under the ability icons: the left side holds the HP/score HUD.
    const int32_t x = game.resources.windowWidth - panelW - px(6);
    int32_t y = px(150); // under the score block

    int32_t directorBosses = 0;
    for (const auto& boss : game.run.bosses)
    {
        directorBosses += static_cast<int32_t>(boss.usesDirector && boss.health > 0);
    }
    const int32_t rows = tacticCount + 1 + (game.autoplay ? 1 : 0) + std::max(1, directorBosses * 2);
    DrawRectangle(x - px(6), y - px(4), panelW, rows * line + px(8),
                  Fade(BLACK, 0.6F));

    if (game.autoplay)
    {
        const std::string botLine = std::string("BOT: ") + autoplayHabitName(game);
        drawText(game, botLine.c_str(), x, y, font, WHITE);
        y += line;
    }
    drawText(game, "Player habits (8s)", x, y, font, Palette::Shield);
    y += line;
    for (int i = 0; i < tacticCount; i++)
    {
        const float s = game.run.tactics.score.at(static_cast<size_t>(i));
        const Color c = s > 0.45F ? Palette::Crit : Palette::StructMid;
        drawText(game, std::string(tacticNames.at(static_cast<size_t>(i))).c_str(), x, y, font, c);
        DrawRectangle(x + labelW, y + px(4), barW, font - px(6), Fade(WHITE, 0.15F));
        DrawRectangle(x + labelW, y + px(4), static_cast<int32_t>(static_cast<float>(barW) * s),
                      font - px(6), c);
        drawText(game, (std::to_string(static_cast<int>(s * 100)) + "%").c_str(),
                 x + labelW + barW + px(8), y, font, c);
        y += line;
    }

    if (directorBosses == 0)
    {
        drawText(game, "No director boss (Spawn Boss)", x, y, font, Palette::StructMid);
        return;
    }
    for (const auto& boss : game.run.bosses)
    {
        if (!boss.usesDirector || boss.health <= 0)
        {
            continue;
        }
        const int hpPct = boss.health * 100 / std::max(1, boss.maxHealth);
        const std::string head = std::string(bossPatternNames.at(static_cast<size_t>(boss.pattern))) +
                                 "  P" + std::to_string(boss.phase + 1) + "  HP " +
                                 std::to_string(hpPct) + "%";
        drawText(game, head.c_str(), x, y, font, Palette::Shield);
        y += line;
        std::string status;
        if (boss.phaseInvulnTimer > 0)
        {
            status = "PHASE SHIFT (invulnerable)";
        }
        else if (boss.dazedTimer > 0)
        {
            status = std::format("DAZED {:.1f}s (x1.5 dmg)", boss.dazedTimer);
        }
        else
        {
            status = std::format("{}  step {}  t {:.1f}s{}", bossAttackNames.at(static_cast<size_t>(boss.attack)),
                                 boss.patternStep, std::max(0.0F, boss.patternTimer),
                                 boss.readTellTimer > 0 ? "  READ YOU" : "");
        }
        drawText(game, status.c_str(), x + px(12), y, font,
                 boss.readTellTimer > 0 ? Palette::Crit : Palette::Accent);
        y += line;
    }
}

void drawSandboxMenu(const Game& game)
{
    applyGuiScale(game);
    windowText(game, "SANDBOX", game.resources.windowWidth / 2, 80, 34, Palette::Accent);

    uiButton(sandboxBrowserModeToggleRect(game), game.sandboxBrowserMode ? "Actions" : "Browser");
    uiButton(sandboxMenuCloseButtonRect(game), "Close [Esc]");

    if (game.sandboxBrowserMode)
    {
        drawSandboxBrowser(game);
        return;
    }

    const auto& steppers = sandboxMenuSteppers();
    for (int32_t i = 0; i < static_cast<int32_t>(steppers.size()); i++)
    {
        const auto& stepper = steppers.at(static_cast<size_t>(i));
        const Rectangle row = sandboxStepperRowRect(game, i);
        // Label sits left of the row, right-aligned and vertically centered on it.
        const float scale = guiUiScale(game);
        const auto labelSize = static_cast<int32_t>(16 * scale);
        const std::string label(stepper.label);
        drawText(game, label.c_str(),
                 static_cast<int32_t>(row.x - 12 * scale) - measureText(game, label.c_str(), labelSize),
                 static_cast<int32_t>(row.y + (row.height - static_cast<float>(labelSize)) / 2),
                 labelSize, Palette::StructLight);

        uiButton(sandboxStepperMinusRect(game, i), "-");
        std::string value = stepper.valueText(game);
        GuiTextBox(sandboxStepperValueRect(game, i), value.data(),
                   static_cast<int32_t>(value.size()) + 1, false);
        uiButton(sandboxStepperPlusRect(game, i), "+");
    }

    const auto& buttons = sandboxMenuButtons();
    for (int32_t i = 0; i < static_cast<int32_t>(buttons.size()); i++)
    {
        uiButton(sandboxMenuButtonRect(game, i),
                  buttons.at(static_cast<size_t>(i)).label(game).c_str());
    }
}

} // namespace DrawDetail
