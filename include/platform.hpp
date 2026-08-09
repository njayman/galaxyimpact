#pragma once

#include <string>

auto getSaveDataDir() -> std::string;

void platformInitSaveData();

auto platformSaveDataReady() -> bool;

void platformSyncSaveData();

void platformExitToLanding();
