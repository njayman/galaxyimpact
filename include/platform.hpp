#pragma once

#include <string>

auto getSaveDataDir() -> std::string;

void platformInitSaveData();

// Non-blocking: on Emscripten, IDBFS mounting/loading happens asynchronously after
// platformInitSaveData() kicks it off. Poll this once per main-loop iteration until it
// returns true before reading any save data. Always true on native builds.
auto platformSaveDataReady() -> bool;

void platformSyncSaveData();

void platformExitToLanding();
