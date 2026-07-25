#include "settings.hpp"

#include "game.hpp"
#include <fstream>
#include <sstream>

namespace
{
constexpr const char* settingsFile = "settings.txt";

// defaultResolutionIndex finds the 1920x1080 entry in resolutionOptions so
// the settings menu starts in sync with the actual default window size.
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
} // namespace

auto loadSettings() -> Settings
{
    Settings settings{.resolutionIndex = defaultResolutionIndex(),
                      .difficulty = Difficulty::Normal,
                      .bgmOn = true,
                      .soundOn = true,
                      .fpsIndex = 0};

    std::ifstream file(settingsFile);
    if (!file)
    {
        return settings;
    }

    int32_t resIdx = 0;
    int32_t difficulty = 0;
    int32_t bgmOn = 0;
    int32_t soundOn = 0;
    if (!(file >> resIdx >> difficulty >> bgmOn >> soundOn))
    {
        return settings;
    }

    if (resIdx >= 0 && static_cast<size_t>(resIdx) < resolutionOptions.size())
    {
        settings.resolutionIndex = resIdx;
    }
    if (difficulty >= 0 && difficulty < static_cast<int32_t>(Difficulty::Count))
    {
        settings.difficulty = static_cast<Difficulty>(difficulty);
    }
    settings.bgmOn = bgmOn != 0;
    settings.soundOn = soundOn != 0;

    // fpsIndex was added after the original 4-field format; older
    // settings.txt files simply won't have it, which is fine (defaults to 0).
    if (int32_t fpsIdx = 0; file >> fpsIdx)
    {
        if (fpsIdx >= 0 && static_cast<size_t>(fpsIdx) < fpsOptions.size())
        {
            settings.fpsIndex = fpsIdx;
        }
    }

    return settings;
}

void saveSettings(const Settings& settings)
{
    std::ofstream file(settingsFile, std::ios::trunc);
    if (!file)
    {
        return;
    }

    file << settings.resolutionIndex << ' ' << static_cast<int32_t>(settings.difficulty) << ' '
         << (settings.bgmOn ? 1 : 0) << ' ' << (settings.soundOn ? 1 : 0) << ' '
         << settings.fpsIndex;
}
