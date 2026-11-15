// libFuzzer harness for the save-file parsers. Byte 0 picks the file (achievements, settings,
// bestiary, highscore); the rest is written as that file's contents and loaded.
// Build: cmake -DGALAXYIMPACT_FUZZ=ON (clang only). Run: ./fuzz_saves -max_total_time=60
#include "achievements.hpp"
#include "bestiary.hpp"
#include "highscore.hpp"
#include "settings.hpp"

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace
{
std::string saveDir;

void writeFile(const std::string& path, const uint8_t* data, size_t size)
{
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    file.write(reinterpret_cast<const char*>(data), static_cast<std::streamsize>(size));
}
} // namespace

extern "C" int LLVMFuzzerInitialize(int* /*argc*/, char*** /*argv*/)
{
    // Keep fuzz writes out of the player's real save directory.
    saveDir = std::filesystem::temp_directory_path().string() + "/galaxyimpact-fuzz-" +
              std::to_string(std::random_device{}());
    std::filesystem::create_directories(saveDir + "/GalaxyImpact");
    setenv("XDG_DATA_HOME", saveDir.c_str(), 1);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0)
    {
        return 0;
    }
    const std::string base = saveDir + "/GalaxyImpact/";
    const uint8_t which = data[0] % 4;
    data += 1;
    size -= 1;
    switch (which)
    {
    case 0:
        writeFile(base + "achievements.txt", data, size);
        (void)loadAchievements();
        break;
    case 1:
        writeFile(base + "settings.txt", data, size);
        (void)loadSettings();
        break;
    case 2:
        writeFile(base + "bestiary.txt", data, size);
        (void)loadBestiary();
        break;
    default:
        writeFile(base + "highscore.txt", data, size);
        (void)highscore::FileRepository(base + "highscore.txt").load();
        break;
    }
    return 0;
}
