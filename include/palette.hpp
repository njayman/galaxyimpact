#pragma once

#include "raylib.h"

namespace Palette
{
constexpr Color Void{.r = 10, .g = 12, .b = 18, .a = 255};
constexpr Color StructDark{.r = 28, .g = 34, .b = 46, .a = 255};
constexpr Color StructMid{.r = 56, .g = 66, .b = 84, .a = 255};
constexpr Color StructLight{.r = 118, .g = 132, .b = 154, .a = 255};
constexpr Color Haze{.r = 150, .g = 165, .b = 185, .a = 255};

constexpr Color Accent{.r = 196, .g = 58, .b = 58, .a = 255};
constexpr Color AccentDim{.r = 140, .g = 40, .b = 40, .a = 255};

constexpr Color Shield{.r = 150, .g = 180, .b = 200, .a = 255};
constexpr Color Charge{.r = 224, .g = 178, .b = 110, .a = 255};
constexpr Color Crit{.r = 255, .g = 226, .b = 170, .a = 255};

constexpr Color BossIdle{.r = 70, .g = 90, .b = 120, .a = 255};
constexpr Color BossHoming{.r = 150, .g = 95, .b = 55, .a = 255};
constexpr Color BossSpread{.r = 140, .g = 70, .b = 95, .a = 255};
constexpr Color BossBeam{.r = 90, .g = 150, .b = 165, .a = 255};

constexpr Color ElementStatic{.r = 225, .g = 240, .b = 255, .a = 255};
constexpr Color ElementFreeze{.r = 90, .g = 210, .b = 230, .a = 255};
constexpr Color ElementBurn{.r = 230, .g = 120, .b = 40, .a = 255};
constexpr Color ElementConfuse{.r = 190, .g = 90, .b = 200, .a = 255};

constexpr Color Heal{.r = 110, .g = 200, .b = 120, .a = 255};

// Biome backgrounds/accents (Void/Haze above stay the default Shattered Belt look).
constexpr Color RustbloomVoid{.r = 16, .g = 14, .b = 10, .a = 255};
constexpr Color RustbloomHaze{.r = 150, .g = 120, .b = 80, .a = 255};
constexpr Color RustbloomAccent{.r = 140, .g = 165, .b = 70, .a = 255};

constexpr Color SolarForgeVoid{.r = 24, .g = 10, .b = 6, .a = 255};
constexpr Color SolarForgeHaze{.r = 230, .g = 140, .b = 60, .a = 255};

constexpr Color PunctumVoid{.r = 6, .g = 6, .b = 10, .a = 255};
constexpr Color PunctumHaze{.r = 200, .g = 150, .b = 230, .a = 255};
constexpr Color PunctumAccent{.r = 230, .g = 200, .b = 60, .a = 255};
}
