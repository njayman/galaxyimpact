#include "draw_internal.hpp"

namespace DrawDetail
{

// Boss QTE cutscene HUD ----------------------------------------------------

void drawQteArrow(Vector2 center, Vector2 dir, float size, Color color)
{
    const Vector2 perp{.x = -dir.y, .y = dir.x};
    const Vector2 tip = Vector2Add(center, Vector2Scale(dir, size));
    const Vector2 back = Vector2Subtract(center, Vector2Scale(dir, size * 0.6F));
    DrawTriangle(tip, Vector2Subtract(back, Vector2Scale(perp, size * 0.7F)),
                 Vector2Add(back, Vector2Scale(perp, size * 0.7F)), color);
    DrawTriangle(tip, Vector2Add(back, Vector2Scale(perp, size * 0.7F)),
                 Vector2Subtract(back, Vector2Scale(perp, size * 0.7F)), color);
}

// A keyboard key that bobs down on the mash beat (press 0..1).
void drawQteKeycap(const Game& game, const char* label, Vector2 center, float width, float press,
                   Color color)
{
    constexpr float height = 56.0F;
    constexpr float depth = 8.0F;
    const float x = center.x - width / 2;
    const float y = center.y - height / 2;
    DrawRectangleRec(Rectangle{.x = x, .y = y + depth, .width = width, .height = height},
                     Fade(BLACK, 0.8F));
    const Rectangle top{.x = x, .y = y + depth * press, .width = width, .height = height};
    DrawRectangleRec(top, Palette::StructDark);
    DrawRectangleLinesEx(top, 3.0F, color);
    const int32_t textWidth = measureText(game, label, 24);
    drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
             static_cast<int32_t>(top.y + height / 2 - 12), 24, color);
}

// A mouse with the asked buttons flashing.
void drawQteMouse(Vector2 center, bool left, bool right, float press, Color color)
{
    constexpr float width = 52.0F;
    constexpr float height = 76.0F;
    const Rectangle body{.x = center.x - width / 2,
                         .y = center.y - height / 2,
                         .width = width,
                         .height = height};
    const float buttonHeight = height * 0.42F;
    DrawRectangleRec(body, Palette::StructDark);
    const Color lit = Fade(color, 0.35F + 0.65F * press);
    if (left)
    {
        DrawRectangleRec(
            Rectangle{.x = body.x, .y = body.y, .width = width / 2, .height = buttonHeight}, lit);
    }
    if (right)
    {
        DrawRectangleRec(Rectangle{.x = center.x, .y = body.y, .width = width / 2,
                                   .height = buttonHeight},
                         lit);
    }
    DrawRectangleLinesEx(body, 3.0F, color);
    DrawLineEx(Vector2{.x = body.x, .y = body.y + buttonHeight},
               Vector2{.x = body.x + width, .y = body.y + buttonHeight}, 3.0F, color);
    DrawLineEx(Vector2{.x = center.x, .y = body.y},
               Vector2{.x = center.x, .y = body.y + buttonHeight}, 3.0F, color);
}

// Touch: a round button named like the on-screen control it stands for.
void drawQteTouchButton(const Game& game, const char* label, Vector2 center, float press,
                        Color color)
{
    const float radius = 38.0F + 4.0F * press;
    DrawCircleV(center, radius, Fade(color, 0.2F + 0.35F * press));
    DrawCircleLinesV(center, radius, color);
    const int32_t textWidth = measureText(game, label, 16);
    drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
             static_cast<int32_t>(center.y) - 8, 16, color);
}

void drawQteButtons(const Game& game, QtePrompt prompt, Vector2 center, float press, Color color)
{
    const bool touch = touchInputActive(game);
    switch (prompt)
    {
    case QtePrompt::Up:
    case QtePrompt::Left:
    case QtePrompt::Right:
    case QtePrompt::Down:
    {
        static constexpr std::array<const char*, 4> keys{"W", "A", "D", "S"};
        static constexpr std::array<Vector2, 4> dirs{
            Vector2{.x = 0, .y = -1}, Vector2{.x = -1, .y = 0}, Vector2{.x = 1, .y = 0},
            Vector2{.x = 0, .y = 1}};
        const auto index = static_cast<size_t>(prompt) - static_cast<size_t>(QtePrompt::Up);
        if (touch)
        {
            // Flick the move stick that way.
            DrawCircleLinesV(center, 40.0F, color);
            drawQteArrow(Vector2Add(center, Vector2Scale(dirs.at(index), 10.0F * press)),
                         dirs.at(index), 24.0F, color);
        }
        else
        {
            drawQteKeycap(game, keys.at(index), center, 56.0F, press, color);
            drawQteArrow(Vector2Add(center, Vector2Scale(dirs.at(index), 62.0F + 8.0F * press)),
                         dirs.at(index), 14.0F, color);
        }
        break;
    }
    case QtePrompt::Shield:
        touch ? drawQteTouchButton(game, "SHIELD", center, press, color)
              : drawQteMouse(center, false, true, press, color);
        break;
    case QtePrompt::Dash:
        touch ? drawQteTouchButton(game, "DASH", center, press, color)
              : drawQteMouse(center, true, false, press, color);
        break;
    case QtePrompt::Parry:
        touch ? drawQteTouchButton(game, "COMBO", center, press, color)
              : drawQteMouse(center, true, true, press, color);
        break;
    case QtePrompt::Nerve:
        touch ? drawQteTouchButton(game, "NERVE", center, press, color)
              : drawQteKeycap(game, "SPACE", center, 170.0F, press, color);
        break;
    case QtePrompt::Mash:
        touch ? drawQteTouchButton(game, "NERVE", center, press, color)
              : drawQteKeycap(game, "SPACE", center, 170.0F, press, color);
        break;
    case QtePrompt::None:
        break;
    }
}

// Cutscene mode: the regular HUD is gone; only letterbox bars, the button to
// mash, its timer and a marker on the side the threat comes from.
void drawBossQteHud(const Game& game)
{
    const float width = static_cast<float>(game.resources.screenWidth) / hudScale(game);
    const float height = static_cast<float>(game.resources.screenHeight) / hudScale(game);
    const float barHeight = height * 0.09F * easeOutCubic(game.qteAge / 0.25F); // letterbox slides in
    DrawRectangleRec(Rectangle{.x = 0, .y = 0, .width = width, .height = barHeight}, BLACK);
    DrawRectangleRec(
        Rectangle{.x = 0, .y = height - barHeight, .width = width, .height = barHeight}, BLACK);

    const QtePrompt prompt = game.run.qtePrompt;
    if (prompt == QtePrompt::None)
    {
        return;
    }

    const auto time = static_cast<float>(GetTime());
    const float press = 0.5F + 0.5F * std::sin(time * 28.0F);
    const bool isAction = prompt >= QtePrompt::Shield;
    const Color color = isAction ? Palette::Charge : Palette::Crit;
    const Vector2 screenCenter{.x = width / 2, .y = height / 2};

    // Threat marker at the edge the attack comes from.
    if (Vector2LengthSqr(game.run.qteThreatDir) > 0)
    {
        const Vector2 from = Vector2Negate(game.run.qteThreatDir);
        const float edge = std::min(width, height) * 0.4F;
        const float blink = 0.5F + 0.5F * std::sin(time * 20.0F);
        drawQteArrow(Vector2Add(screenCenter, Vector2Scale(from, edge)), game.run.qteThreatDir,
                     20.0F, Fade(Palette::Crit, 0.5F + 0.5F * blink));
    }

    const Vector2 center{.x = screenCenter.x, .y = height * 0.3F};
    // Each new prompt pops in; untimed ones (Mash) pop once with the QTE.
    const float promptAge = game.run.qteTimerMax > 0 ? game.run.qteTimerMax - game.run.qteTimer : game.qteAge;
    beginPop(center, easeOutBack(promptAge / 0.16F));
    drawQteButtons(game, prompt, center, press, color);
    endPop();

    if (game.run.qteTimerMax > 0)
    {
        constexpr float timerWidth = 180.0F;
        const float frac = std::clamp(game.run.qteTimer / game.run.qteTimerMax, 0.0F, 1.0F);
        const float y = center.y + 70.0F;
        DrawRectangleRec(Rectangle{.x = center.x - timerWidth / 2, .y = y, .width = timerWidth,
                                   .height = 6.0F},
                         Fade(Palette::StructMid, 0.5F));
        DrawRectangleRec(Rectangle{.x = center.x - timerWidth / 2 * frac, .y = y,
                                   .width = timerWidth * frac, .height = 6.0F},
                         color);
    }

    if (prompt == QtePrompt::Mash && game.run.qteMashTarget > 0)
    {
        constexpr float mashWidth = 260.0F;
        constexpr float mashHeight = 16.0F;
        const float frac = std::clamp(static_cast<float>(game.run.qteMashCount) /
                                          static_cast<float>(game.run.qteMashTarget),
                                      0.0F, 1.0F);
        const Rectangle bar{.x = center.x - mashWidth / 2, .y = center.y + 86.0F, .width = mashWidth,
                            .height = mashHeight};
        DrawRectangleRec(bar, Fade(Palette::StructMid, 0.5F));
        DrawRectangleRec(Rectangle{.x = bar.x, .y = bar.y, .width = mashWidth * frac, .height = mashHeight},
                         color);
        DrawRectangleLinesEx(bar, 2.0F, color);
        const int32_t textWidth = measureText(game, "MASH!", 20);
        drawText(game, "MASH!", static_cast<int32_t>(center.x) - textWidth / 2,
                 static_cast<int32_t>(bar.y + mashHeight + 8.0F), 20, color);
    }
}

void drawHUD(const Game& game)
{
    if (bossQteActive(game))
    {
        drawBossQteHud(game);
        return;
    }

    const auto width = static_cast<int32_t>(static_cast<float>(game.resources.screenWidth) / hudScale(game));
    const auto height = static_cast<int32_t>(static_cast<float>(game.resources.screenHeight) / hudScale(game));
    constexpr int32_t margin = 22;

    // XP: a hairline along the very top edge.
    const float xpFrac = std::min(static_cast<float>(game.run.xp) / static_cast<float>(game.run.xpToNext), 1.0F);
    DrawRectangle(0, 0, width, 4, Fade(Palette::StructMid, 0.6F));
    DrawRectangle(0, 0, static_cast<int32_t>(static_cast<float>(width) * xpFrac), 4, Palette::Charge);

    drawOffscreenBosses(game, width, height);
    const int32_t vitalsBottom = drawVitals(game, margin, margin);
    drawStatusRow(game, margin, vitalsBottom + 10);
    drawRunNumbers(game, width - margin, margin - 6);
    drawBossLines(game, width, height);
    drawAbilitySlots(game, margin, height - margin - 54);

    // Controls: below the ship, clear of the HUD, only at the start of a run, then gone.
    constexpr float hintTime = 8.0F;
    if (game.run.runTime < hintTime && game.run.bosses.empty() && game.state == GameState::GAMEPLAY)
    {
        const char* controlHint =
            touchInputActive(game)
                ? "Move: Left Stick | Aim: Right Stick | Shield/Combo/Dash: Either Panel | Burst: Nerve | Pause: Top Button"
                : "Move: WASD | Dash: L-Click | Shield: R-Click | Burst: Space | Pause: Esc | F11: Fullscreen";
        const float alpha = std::clamp((hintTime - game.run.runTime) / 2.0F, 0.0F, 1.0F);
        constexpr int32_t hintSize = 14;
        drawText(game, controlHint, (width - measureText(game, controlHint, hintSize)) / 2, height * 3 / 4,
                 hintSize, Fade(Palette::StructLight, 0.7F * alpha));
    }
}

void drawTouchControls(const Game& game)
{
    if (!game.touch.visible || game.autoplay)
    {
        return;
    }

    const auto& touch = game.touch;
    const TouchLayout layout = touchControlLayout(game);
    const auto fontSize = [&](float base) -> int32_t
    { return static_cast<int32_t>(base * guiUiScale(game)); };

    // Very faint - the stick's own track shouldn't compete visually with the
    // gameplay behind it, only the knob (your thumb's actual position) needs
    // to read clearly.
    const auto drawStick = [&](const TouchStick& stick, Vector2 anchor)
    {
        const Vector2 knob = Vector2Add(anchor, stick.offset);
        const bool active = stick.touchId != -1;

        DrawCircleV(anchor, layout.stickRadius, Fade(Palette::StructMid, 0.10F));
        DrawCircleLinesV(anchor, layout.stickRadius, Fade(Palette::StructLight, 0.10F));
        DrawCircleV(knob, layout.stickRadius * 0.45F, Fade(Palette::Accent, active ? 0.55F : 0.3F));
    };

    drawStick(touch.moveStick, layout.moveStickAnchor);
    drawStick(touch.aimStick, layout.aimStickAnchor);

    const auto drawCircleButton = [&](const TouchButton& button, Vector2 center, float radius,
                                      const char* label)
    {
        DrawCircleV(
            center, radius,
            Fade(button.down ? Palette::Accent : Palette::StructMid, button.down ? 0.55F : 0.3F));
        DrawCircleLinesV(center, radius, Fade(Palette::StructLight, 0.6F));
        const int32_t size = fontSize(18.0F);
        const int32_t textWidth = measureText(game, label, size);
        drawText(game, label, static_cast<int32_t>(center.x) - textWidth / 2,
                 static_cast<int32_t>(center.y) - size / 2, size,
                 button.down ? Palette::Accent : Palette::StructLight);
    };

    drawCircleButton(touch.nerve, layout.nerveCenter, layout.nerveRadius, "NERVE");

    // Dash/shield/combo: three separate chamfered-corner button panels per
    // side (shield/dash stacked, combo as its own full-height bar next to
    // them) - real distinct filled buttons, not one connected outline shape.
    // Filled solid with the current ship's own color (brightened when held);
    // outlined by drawing the same chamfered shape slightly larger in a dark
    // color first, then the true-size fill on top.
    const Color shipColor = currentShip(game).color;
    const float chamfer = layout.buttonChamfer;
    const float outlineExtra = 4.0F * guiUiScale(game);

    const auto chamferPoints = [](Rectangle r, float c) -> std::array<Vector2, 8>
    {
        const float cc = std::min(c, std::min(r.width, r.height) / 2.0F);
        const float x0 = r.x;
        const float y0 = r.y;
        const float x1 = r.x + r.width;
        const float y1 = r.y + r.height;
        return {
            Vector2{.x = x0 + cc, .y = y0}, Vector2{.x = x1 - cc, .y = y0},
            Vector2{.x = x1, .y = y0 + cc}, Vector2{.x = x1, .y = y1 - cc},
            Vector2{.x = x1 - cc, .y = y1}, Vector2{.x = x0 + cc, .y = y1},
            Vector2{.x = x0, .y = y1 - cc}, Vector2{.x = x0, .y = y0 + cc},
        };
    };
    const auto drawButtonPanel = [&](Rectangle rect, bool down)
    {
        const Color fillColor = down ? ColorBrightness(shipColor, 0.35F) : shipColor;
        const Rectangle outlineRect{.x = rect.x - outlineExtra,
                                    .y = rect.y - outlineExtra,
                                    .width = rect.width + outlineExtra * 2,
                                    .height = rect.height + outlineExtra * 2};
        auto outlinePts = chamferPoints(outlineRect, chamfer + outlineExtra);
        DrawTriangleFan(outlinePts.data(), static_cast<int32_t>(outlinePts.size()),
                        Palette::StructDark);
        auto fillPts = chamferPoints(rect, chamfer);
        DrawTriangleFan(fillPts.data(), static_cast<int32_t>(fillPts.size()), fillColor);
    };
    drawButtonPanel(layout.leftShieldRect, touch.leftShield.down);
    drawButtonPanel(layout.leftComboRect, touch.leftCombo.down);
    drawButtonPanel(layout.leftDashRect, touch.leftDash.down);
    drawButtonPanel(layout.rightShieldRect, touch.rightShield.down);
    drawButtonPanel(layout.rightComboRect, touch.rightCombo.down);
    drawButtonPanel(layout.rightDashRect, touch.rightDash.down);

    DrawCircleV(layout.pauseCenter, layout.pauseRadius, Fade(Palette::StructMid, 0.3F));
    DrawCircleLinesV(layout.pauseCenter, layout.pauseRadius, Fade(Palette::StructLight, 0.6F));
    const float pauseBarWidth = layout.pauseRadius * 0.22F;
    const float pauseBarHeight = layout.pauseRadius * 0.9F;
    const float pauseBarGap = layout.pauseRadius * 0.18F;
    DrawRectangle(static_cast<int32_t>(layout.pauseCenter.x - pauseBarGap - pauseBarWidth),
                  static_cast<int32_t>(layout.pauseCenter.y - pauseBarHeight / 2),
                  static_cast<int32_t>(pauseBarWidth), static_cast<int32_t>(pauseBarHeight),
                  Palette::StructLight);
    DrawRectangle(static_cast<int32_t>(layout.pauseCenter.x + pauseBarGap),
                  static_cast<int32_t>(layout.pauseCenter.y - pauseBarHeight / 2),
                  static_cast<int32_t>(pauseBarWidth), static_cast<int32_t>(pauseBarHeight),
                  Palette::StructLight);
}

// --- Minimal HUD (docs/design/ui.md): bare lines and small numbers at the edges ---------

// Vitals dim to this when nothing has changed for a while.
auto hudCalmAlpha(const Game& game) -> float
{
    const float t = std::clamp((game.run.hud.calmTimer - 3.0F) / 1.5F, 0.0F, 1.0F);
    return 1.0F - 0.2F * t;
}

auto withThousands(int32_t value) -> std::string
{
    std::string digits = std::to_string(value);
    for (auto i = static_cast<int32_t>(digits.size()) - 3; i > 0; i -= 3)
    {
        digits.insert(static_cast<size_t>(i), ",");
    }
    return digits;
}

// HP as one full bar with the count beside it, Nerve as a hairline under it, dash charges and shield
// stacks as pips after the count. Returns the bottom y.
auto drawVitals(const Game& game, int32_t x, int32_t y) -> int32_t
{
    const Player& player = game.run.player;
    const HudState& hud = game.run.hud;
    const float alpha = hudCalmAlpha(game);
    const auto time = static_cast<float>(GetTime());
    constexpr float hpWidth = 190.0F;
    const bool low = player.health <= player.maxHealth / 3.0F;
    const float lowPulse = low ? 0.5F + 0.5F * std::sin(time * 7.0F) : 0.0F;
    const auto fx = static_cast<float>(x);
    const auto fy = static_cast<float>(y);

    // One full bar; the part lost to the last hit flashes white, then fades.
    const float maxHp = std::max(1.0F, player.maxHealth);
    const float fill = std::clamp(player.health / maxHp, 0.0F, 1.0F);
    DrawRectangleRec(Rectangle{.x = fx, .y = fy, .width = hpWidth, .height = 8}, Fade(Palette::StructDark, alpha));
    const Color hp = ColorLerp(Palette::Accent, Palette::Crit, lowPulse * 0.6F);
    DrawRectangleRec(Rectangle{.x = fx, .y = fy, .width = hpWidth * fill, .height = 8}, Fade(hp, alpha));
    const float lost = std::clamp(hud.hitFrom / maxHp, 0.0F, 1.0F) - fill;
    if (hud.hitFlashTimer > 0 && lost > 0)
    {
        DrawRectangleRec(Rectangle{.x = fx + hpWidth * fill, .y = fy - 1, .width = hpWidth * lost, .height = 10},
                         Fade(WHITE, hud.hitFlashTimer / 0.6F));
    }
    const std::string count = std::format("{:.0f}", std::ceil(player.health));
    drawText(game, count.c_str(), static_cast<int32_t>(fx + hpWidth + 8), y - 4, 16,
             Fade(low ? Palette::Crit : Palette::Haze, alpha));

    // Nerve hairline.
    const float frac = nerveFrac(game);
    const bool ready = frac >= 1.0F;
    const float readyPulse = 0.5F + 0.5F * std::sin(time * 8.0F);
    const float ny = fy + 13;
    DrawRectangleRec(Rectangle{.x = fx, .y = ny, .width = hpWidth, .height = 3.0F}, Fade(Palette::StructDark, alpha));
    const Color nerve = ready ? ColorLerp(Palette::Charge, WHITE, readyPulse * 0.5F)
                              : ColorLerp(Palette::Charge, Palette::Crit, frac);
    DrawRectangleRec(Rectangle{.x = fx, .y = ny, .width = hpWidth * frac, .height = 3.0F},
                     Fade(nerve, ready ? 1.0F : alpha));
    if (player.nerveCharging)
    {
        const float charge = 1.0F - player.nerveChargeTimer / UpdateConstants::nerveBurstWindup;
        DrawRectangleRec(Rectangle{.x = fx, .y = ny - 1, .width = hpWidth * charge, .height = 5.0F}, Palette::Crit);
    }
    else if (ready)
    {
        drawText(game, "SPACE", static_cast<int32_t>(fx + hpWidth + 8), static_cast<int32_t>(ny) - 5, 14,
                 Fade(Palette::Charge, 0.5F + 0.5F * readyPulse));
    }

    // Dash charges (dots, the next one refilling as a ring) and shield stacks (small chevrons).
    float px = fx + 5;
    const float py = ny + 15;
    for (int32_t i = 0; i < UpdateConstants::maxCharges; i++, px += 16)
    {
        const Vector2 c{.x = px, .y = py};
        if (i < player.charges)
        {
            DrawCircleV(c, 5.0F, Fade(Palette::Shield, alpha));
            continue;
        }
        DrawCircleV(c, 5.0F, Fade(Palette::StructMid, alpha)); // empty: flat dim fill, no outline
        if (i == player.charges)
        {
            const float d = chargeRegenDuration(game);
            const float progress = d > 0 ? 1 - player.chargeRegenTimer / d : 1;
            DrawRing(c, 2.5F, 5.0F, -90, -90 + 360 * progress, 16,
                     Fade(isNerveChargeFeeding(game) ? Palette::Charge : Palette::Shield, alpha));
        }
    }
    px += 6;
    for (int32_t i = 0; i < maxShieldStacks(game); i++, px += 16)
    {
        const bool full = i < player.shieldStacks;
        const Vector2 a{.x = px - 5.5F, .y = py - 4.0F};
        const Vector2 b{.x = px + 5.5F, .y = py - 4.0F};
        const Vector2 tip{.x = px, .y = py + 5.0F};
        if (full)
        {
            DrawTriangle(tip, b, a, Fade(Palette::Shield, 0.85F * alpha));
        }
        else
        {
            DrawTriangle(tip, b, a, Fade(Palette::StructMid, alpha));
        }
    }
    return static_cast<int32_t>(py + 7);
}

// Top right, right-aligned: score, then wave and level, DMG, the biome name when it changes,
// and the toast.
void drawRunNumbers(const Game& game, int32_t right, int32_t y)
{
    const auto line = [&](const std::string& text, int32_t size, Color color, int32_t yy)
    { drawText(game, text.c_str(), right - measureText(game, text.c_str(), size), yy, size, color); };

    line(withThousands(game.run.score), 24, Palette::Haze, y);
    y += 26;
    line(std::format("WAVE {}   LV {}", game.run.waveNumber, game.run.level), 15, Palette::StructLight, y);
    y += 19;
    const auto& meter = game.run.damageMeterDisplay;
    if (meter.total > 0)
    {
        line(std::format("DMG {}", withThousands(meter.total)), 13, Fade(Palette::Crit, 0.85F), y);
        y += 17;
    }
    if (game.run.hud.biomeTimer > 0)
    {
        const float t = game.run.hud.biomeTimer;
        const float alpha = std::min({1.0F, (4.0F - t) / 0.5F, t / 1.0F});
        std::string name(biomeName(static_cast<Biome>(game.run.hud.biome)));
        std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        line(name, 13, Fade(Palette::Haze, alpha), y);
        y += 17;
    }
    if (game.run.achievementToastTimer > 0)
    {
        const float t = game.run.achievementToastTimer;
        const float alpha = std::min(1.0F, t / 0.6F);
        const auto drift = static_cast<int32_t>((1.0F - alpha) * -8.0F);
        line(game.run.achievementToast, 15, Fade(Palette::Charge, alpha), y + 6 + drift);
    }
}

// A boss's fixed HUD colour: its bestiary colour, dark hulls lifted so it reads on the void.
auto bossLineColor(const Boss& boss) -> Color
{
    const Color color = bossBestiaryColor(bossBestiaryIndex(boss));
    return color.r + color.g + color.b < 330 ? ColorBrightness(color, 0.45F) : color;
}

// A chevron on the screen edge for each boss out of view, pointing at it, in its line colour;
// it grows as the boss closes in, with the distance beside it.
void drawOffscreenBosses(const Game& game, int32_t width, int32_t height)
{
    const float scale = hudScale(game);
    const Vector2 centre{.x = static_cast<float>(width) / 2, .y = static_cast<float>(height) / 2};
    constexpr float inset = 30.0F;
    const auto pulse = 0.75F + 0.25F * std::sin(static_cast<float>(GetTime()) * 5.0F);
    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0 || boss.isBeltbreakerPlate || boss.isWreckwormSegment)
        {
            continue;
        }
        const Vector2 bossCentre{.x = boss.position.x + boss.size.x / 2, .y = boss.position.y + boss.size.y / 2};
        const Vector2 d = Vector2Scale(Vector2Subtract(bossCentre, game.run.player.position), 1.0F / scale);
        const float reach = std::max(boss.size.x, boss.size.y) / 2 / scale;
        if (std::abs(d.x) - reach < centre.x && std::abs(d.y) - reach < centre.y)
        {
            continue; // some of it is on screen
        }
        const float t = std::min((centre.x - inset) / std::max(std::abs(d.x), 1.0F),
                                 (centre.y - inset) / std::max(std::abs(d.y), 1.0F));
        const Vector2 p = Vector2Add(centre, Vector2Scale(d, t));
        const Vector2 dir = Vector2Normalize(d);
        const Vector2 side{.x = -dir.y, .y = dir.x};
        const float dist = Vector2Length(d) * scale;
        const float size = std::clamp(32.0F - dist / 200.0F, 20.0F, 32.0F);
        const Color color = Fade(ColorBrightness(bossLineColor(boss), 0.25F), pulse);
        DrawCircleV(p, size * 0.85F, Fade(Palette::Void, 0.7F)); // dark backing so any colour reads
        DrawCircleLines(static_cast<int32_t>(p.x), static_cast<int32_t>(p.y), size * 0.85F, Fade(color, 0.5F));

        const Vector2 tip = Vector2Add(p, Vector2Scale(dir, size * 0.6F));
        const Vector2 notch = Vector2Subtract(p, Vector2Scale(dir, size * 0.1F));
        const Vector2 left = Vector2Add(Vector2Subtract(p, Vector2Scale(dir, size * 0.5F)), Vector2Scale(side, size * 0.6F));
        const Vector2 right = Vector2Subtract(Vector2Subtract(p, Vector2Scale(dir, size * 0.5F)), Vector2Scale(side, size * 0.6F));
        DrawTriangle(tip, left, notch, color); // a chevron: two halves, either winding
        DrawTriangle(tip, notch, left, color);
        DrawTriangle(tip, right, notch, color);
        DrawTriangle(tip, notch, right, color);

        const std::string label = std::to_string(static_cast<int32_t>(dist / 10.0F));
        const Vector2 at = Vector2Subtract(p, Vector2Scale(dir, size + 12.0F));
        drawText(game, label.c_str(), static_cast<int32_t>(at.x) - measureText(game, label.c_str(), 12) / 2,
                 static_cast<int32_t>(at.y) - 6, 12, Fade(Palette::Haze, 0.8F));
    }
}

// One hairline per boss along the bottom centre: name in small caps above, a pale trail of
// recent damage behind the fill, ticks where director bosses change phase.
void drawBossLines(const Game& game, int32_t width, int32_t height)
{
    constexpr float lineWidth = 560.0F;
    const float x = (static_cast<float>(width) - lineWidth) / 2;
    float y = static_cast<float>(height) - 34;
    const auto label = [&](const std::string& left, const std::string& right, Color color)
    {
        drawText(game, left.c_str(), static_cast<int32_t>(x), static_cast<int32_t>(y) - 18, 13, color);
        if (!right.empty())
        {
            drawText(game, right.c_str(), static_cast<int32_t>(x + lineWidth) - measureText(game, right.c_str(), 13),
                     static_cast<int32_t>(y) - 18, 13, color);
        }
    };

    if (currentBiome(game.run.waveNumber) == Biome::SolarForge)
    {
        const float heat = std::clamp(game.run.solarForgeHeatMeter / solarForgeMeltThreshold, 0.0F, 1.0F);
        const bool melting = game.run.solarForgeHeatMeter > solarForgeMeltThreshold;
        const float pulse = 0.6F + 0.4F * std::sin(static_cast<float>(GetTime()) * 10.0F);
        DrawRectangleRec(Rectangle{.x = x, .y = y, .width = lineWidth, .height = 2}, Fade(Palette::StructDark, 0.9F));
        DrawRectangleRec(Rectangle{.x = x, .y = y, .width = lineWidth * heat, .height = 2},
                         ColorLerp(Palette::ElementBurn, Palette::Crit, heat));
        label("HEAT", melting ? "MELTING" : "", melting ? Fade(Palette::Crit, pulse) : Palette::StructMid);
        y -= 34;
    }

    for (const auto& boss : game.run.bosses)
    {
        if (boss.health <= 0 || boss.isBeltbreakerPlate || boss.isWreckwormSegment)
        {
            continue;
        }
        const auto maxHp = static_cast<float>(boss.maxHealth);
        const float frac = std::clamp(static_cast<float>(boss.health) / maxHp, 0.0F, 1.0F);
        const float trail = std::clamp(std::max(boss.trailHealth, 0.0F) / maxHp, frac, 1.0F);
        DrawRectangleRec(Rectangle{.x = x, .y = y, .width = lineWidth, .height = 3}, Fade(Palette::StructDark, 0.9F));
        DrawRectangleRec(Rectangle{.x = x + lineWidth * frac, .y = y, .width = lineWidth * (trail - frac), .height = 3},
                         Fade(Palette::Crit, 0.55F));
        const int32_t index = bossBestiaryIndex(boss);
        const Color color = bossLineColor(boss);
        DrawRectangleRec(Rectangle{.x = x, .y = y, .width = lineWidth * frac, .height = 3}, color);
        if (boss.usesDirector)
        {
            for (const float tick : {0.5F, 0.2F})
            {
                DrawRectangleRec(Rectangle{.x = x + lineWidth * tick - 0.5F, .y = y - 3, .width = 1, .height = 9},
                                 Palette::StructLight);
            }
        }
        std::string name(bossBestiaryName(index));
        std::ranges::transform(name, name.begin(), [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        const bool shielded = boss.isBeltbreaker && boss.beltbreakerShielded;
        label(name, shielded ? "SHIELDED" : "", Palette::Haze);
        y -= 34;
    }
}

enum class StatusIconKind : std::uint8_t
{
    Dot,
    Weapon,
    Exclaim
};

struct StatusEntry
{
    StatusIconKind iconKind;
    Color color;
    WeaponType weaponType = WeaponType::Forward;
    float fracRemaining = -1;
    std::string label;
};

void drawStatusRing(Vector2 center, float radius, float frac, Color color)
{

    constexpr float ringThickness = 2.0F;
    DrawRing(center, radius, radius + ringThickness, 0, 360, 24, Fade(Palette::StructDark, 0.7F));
    if (frac > 0)
    {
        const float endAngle = -90.0F + 360.0F * std::clamp(frac, 0.0F, 1.0F);
        DrawRing(center, radius, radius + ringThickness, -90.0F, endAngle, 24, color);
    }
}

void drawStatusIcon(const Game& game, const StatusEntry& entry, Vector2 center, float radius)
{
    switch (entry.iconKind)
    {
    case StatusIconKind::Dot:
        DrawCircleV(center, radius, entry.color);
        break;
    case StatusIconKind::Weapon:
        drawWeaponIcon(entry.weaponType, center, radius, entry.color);
        break;
    case StatusIconKind::Exclaim:
        DrawCircleV(center, radius, entry.color);
        drawText(game, "!", static_cast<int32_t>(center.x) - 3, static_cast<int32_t>(center.y) - 7,
                 14, Palette::Void);
        break;
    }
}

void drawStatusRow(const Game& game, int32_t x, int32_t y)
{
    const auto& player = game.run.player;
    std::vector<StatusEntry> entries;

    for (size_t e = 0; e < static_cast<size_t>(ElementType::Count); e++)
    {
        const float timer = player.elementalBuffTimer.at(e);
        if (timer <= 0)
        {
            continue;
        }
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Dot,
                        .color = elementColors.at(e),
                        .fracRemaining = timer / pickupEffectDuration,
                        .label = std::format("{} {:.0f}s", elementNames.at(e), std::ceil(timer))});
    }

    if (player.regenTimer > 0)
    {
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Dot,
                        .color = Palette::Accent,
                        .fracRemaining = player.regenTimer / pickupEffectDuration,
                        .label = std::format("Regen {:.0f}s", std::ceil(player.regenTimer))});
    }

    if (player.overchargeTimer > 0)
    {
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Dot,
            .color = Palette::Charge,
            .fracRemaining = player.overchargeTimer / overchargeDuration,
            .label = std::format("Overcharge {:.0f}s", std::ceil(player.overchargeTimer))});
    }

    if (player.secondWindReady)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Dot,
                                      .color = Palette::Crit,
                                      .fracRemaining = -1,
                                      .label = "Second Wind ready"});
    }

    if (player.overdriveTimer > 0)
    {
        entries.push_back(StatusEntry{
            .iconKind = StatusIconKind::Dot,
            .color = Palette::Crit,
            .fracRemaining = player.overdriveTimer / overdriveDuration,
            .label = std::format("Overdrive {:.0f}s", std::ceil(player.overdriveTimer))});
    }

    if (player.blindTimer > 0)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Exclaim,
                                      .color = Palette::Accent,
                                      .fracRemaining = player.blindTimer / blindDuration,
                                      .label = std::format("Blinded {:.0f}s", std::ceil(player.blindTimer))});
    }
    if (player.frenzyTimer > 0)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Exclaim,
                                      .color = Palette::Accent,
                                      .fracRemaining = player.frenzyTimer / frenzyDuration,
                                      .label = std::format("Enemy frenzy {:.0f}s", std::ceil(player.frenzyTimer))});
    }

    if (game.run.weaponDowngrade.has_value())
    {
        const auto& downgrade = *game.run.weaponDowngrade;
        entries.push_back(
            StatusEntry{.iconKind = StatusIconKind::Weapon,
                        .color = Palette::Accent,
                        .weaponType = downgrade.type,
                        .fracRemaining = downgrade.timer / hackDuration,
                        .label = std::format("{} hacked {:.0f}s", weaponDisplayName(downgrade.type),
                                             std::ceil(downgrade.timer))});
    }

    const bool suppressed = std::any_of(
        game.run.eliteHazards.begin(), game.run.eliteHazards.end(), [](const EliteHazard& hazard)
        { return hazard.active && hazard.role == EliteHazardRole::Suppressor; });
    if (suppressed)
    {
        entries.push_back(StatusEntry{.iconKind = StatusIconKind::Exclaim,
                                      .color = Palette::Shield,
                                      .fracRemaining = -1,
                                      .label = "Suppressed: weapon cooldowns +40%"});
    }

    if (entries.empty())
    {
        return;
    }

    constexpr float iconRadius = 8.0F;
    constexpr float iconGap = 24.0F;
    const Vector2 mouse = mouseUIPos(game);
    std::optional<size_t> hoveredIdx;

    for (size_t i = 0; i < entries.size(); i++)
    {
        const Vector2 center{.x = static_cast<float>(x) + iconRadius +
                                  static_cast<float>(i) * iconGap,
                             .y = static_cast<float>(y) + iconRadius};
        const auto& entry = entries.at(i);
        if (entry.fracRemaining >= 0)
        {
            drawStatusRing(center, iconRadius, entry.fracRemaining, entry.color);
        }
        drawStatusIcon(game, entry, center, iconRadius * 0.72F);

        if (hoverActive(game) && CheckCollisionPointCircle(mouse, center, iconRadius + 4))
        {
            hoveredIdx = i;
        }
    }

    if (hoveredIdx.has_value())
    {
        const auto& entry = entries.at(*hoveredIdx);
        const Vector2 center{.x = static_cast<float>(x) + iconRadius +
                                  static_cast<float>(*hoveredIdx) * iconGap,
                             .y = static_cast<float>(y) + iconRadius};
        constexpr int32_t tipFontSize = 14;
        const int32_t boxWidth = measureText(game, entry.label.c_str(), tipFontSize) + 14;
        const auto tipX = static_cast<int32_t>(center.x) - boxWidth / 2;
        const auto tipY = static_cast<int32_t>(center.y) + static_cast<int32_t>(iconRadius) + 6;

        DrawRectangle(tipX, tipY, boxWidth, 22, Fade(Palette::Void, 0.9F));
        DrawRectangleLines(tipX, tipY, boxWidth, 22, entry.color);
        drawText(game, entry.label.c_str(), tipX + 7, tipY + 4, tipFontSize, Palette::StructLight);
    }
}

} // namespace DrawDetail
