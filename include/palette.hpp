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
}
