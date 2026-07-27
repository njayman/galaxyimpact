#pragma once

#include <string>

auto getSaveDataDir() -> std::string;

void platformInitSaveData();

void platformSyncSaveData();

void platformExitToLanding();
