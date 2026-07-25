#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace highscore
{

class Repository
{
  public:
    Repository() = default;
    Repository(const Repository&) = default;
    Repository(Repository&&) = default;
    auto operator=(const Repository&) -> Repository& = default;
    auto operator=(Repository&&) -> Repository& = default;
    virtual ~Repository() = default;

    virtual auto load() -> std::vector<int32_t> = 0;
    virtual auto save(const std::vector<int32_t>& scores) -> bool = 0;
};

class FileRepository : public Repository
{
  public:
    explicit FileRepository(std::string path);

    auto load() -> std::vector<int32_t> override;
    auto save(const std::vector<int32_t>& scores) -> bool override;

  private:
    std::string path;
};

// Record inserts score into scores, keeps it sorted descending, truncates to
// the top maxEntries, persists via repo, and returns the updated list.
auto record(Repository& repo, const std::vector<int32_t>& scores,
            int32_t score) -> std::vector<int32_t>;

} // namespace highscore
