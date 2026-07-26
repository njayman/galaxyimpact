// Single translation unit that generates raygui's function bodies - every
// other file just includes "raygui.h" for declarations (see guitheme.hpp).
#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
