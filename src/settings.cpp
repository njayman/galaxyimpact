#include "settings.hpp"

#include "entities/ship.hpp"
#include "game.hpp"
#include "platform.hpp"
#include <fstream>

namespace
{
auto settingsFilePath() -> std::string { return getSaveDataDir() + "settings.txt"; }

auto defaultResolutionIndex() -> int32_t
{
    for (size_t i = 0; i < resolutionOptions.size(); i++)
    {
        const auto& res = resolutionOptions.at(i);
        if (res.width == GameConstants::defaultWindowWidth &&
            res.height == GameConstants::defaultWindowHeight)
        {
            return static_cast<int32_t>(i);
        }
    }
    return 0;
}
}

auto loadSettings() -> Settings
{
    Settings settings{.resolutionIndex = defaultResolutionIndex(),
                      .bgmOn = true,
                      .soundOn = true,
                      .fpsIndex = 0,
                      .hudScaleIndex = defaultHudScaleIndex,
                      .shipIndex = 1};

    std::ifstream file(settingsFilePath());
    if (!file)
    {
        return settings;
    }

    int32_t resIdx = 0;
    // Legacy positional slot: used to hold the Easy/Normal/Hard difficulty index. The difficulty
    // system is gone, but this read must stay so old save files don't misalign the fields after
    // it (bgmOn, soundOn, fpsIndex, ...).
    int32_t legacyDifficulty = 0;
    int32_t bgmOn = 0;
    int32_t soundOn = 0;
    if (!(file >> resIdx >> legacyDifficulty >> bgmOn >> soundOn))
    {
        return settings;
    }

    if (resIdx >= 0 && static_cast<size_t>(resIdx) < resolutionOptions.size())
    {
        settings.resolutionIndex = resIdx;
    }
    settings.bgmOn = bgmOn != 0;
    settings.soundOn = soundOn != 0;

    if (int32_t fpsIdx = 0; file >> fpsIdx)
    {
        if (fpsIdx >= 0 && static_cast<size_t>(fpsIdx) < fpsOptions.size())
        {
            settings.fpsIndex = fpsIdx;
        }
    }

    if (int32_t hudScaleIdx = 0; file >> hudScaleIdx)
    {
        if (hudScaleIdx >= 0 && static_cast<size_t>(hudScaleIdx) < hudScaleOptions.size())
        {
            settings.hudScaleIndex = hudScaleIdx;
        }
    }

    if (int32_t shipIdx = 0; file >> shipIdx)
    {
        if (shipIdx >= 0 && static_cast<size_t>(shipIdx) < static_cast<size_t>(ShipClass::Count))
        {
            settings.shipIndex = shipIdx;
        }
    }

    return settings;
}

void saveSettings(const Settings& settings)
{
    std::ofstream file(settingsFilePath(), std::ios::trunc);
    if (!file)
    {
        return;
    }

    // The '0' below is a placeholder for the removed difficulty field, kept only so this
    // positional slot lines up with legacy save files loadSettings() may still need to read.
    file << settings.resolutionIndex << ' ' << 0 << ' ' << (settings.bgmOn ? 1 : 0) << ' '
         << (settings.soundOn ? 1 : 0) << ' ' << settings.fpsIndex << ' ' << settings.hudScaleIndex
         << ' ' << settings.shipIndex;
    file.close();
    platformSyncSaveData();
}
