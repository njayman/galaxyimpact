#include "highscore.hpp"

#include "platform.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>

namespace highscore
{

namespace
{
constexpr size_t maxEntries = 5;
}

FileRepository::FileRepository(std::string path) : path(std::move(path)) {}

auto FileRepository::load() -> std::vector<int32_t>
{
    std::ifstream file(path);
    if (!file)
    {
        return {};
    }

    std::vector<int32_t> scores;
    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
        {
            continue;
        }
        int32_t value = 0;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        const auto [ptr, ec] = std::from_chars(line.data(), line.data() + line.size(), value);
        if (ec == std::errc{})
        {
            scores.push_back(value);
        }
    }

    return scores;
}

auto FileRepository::save(const std::vector<int32_t>& scores) -> bool
{
    std::ofstream file(path, std::ios::trunc);
    if (!file)
    {
        return false;
    }

    for (size_t i = 0; i < scores.size(); i++)
    {
        if (i > 0)
        {
            file << '\n';
        }
        file << scores.at(i);
    }

    const bool ok = static_cast<bool>(file);
    file.close();
    if (ok)
    {
        platformSyncSaveData();
    }
    return ok;
}

auto record(Repository& repo, const std::vector<int32_t>& scores,
            int32_t score) -> std::vector<int32_t>
{
    std::vector<int32_t> updated = scores;
    updated.push_back(score);

    std::ranges::sort(updated, std::ranges::greater{});

    if (updated.size() > maxEntries)
    {
        updated.resize(maxEntries);
    }

    repo.save(updated);

    return updated;
}

} // namespace highscore
