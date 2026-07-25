#include "platform.hpp"

#include <cstdlib>
#include <filesystem>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

auto getSaveDataDir() -> std::string { return "/save/"; }

namespace
{
// Flipped by onIdbfsSynced once the browser's IndexedDB round-trip
// (mount+load, or a later save) actually completes - syncfs is asynchronous
// in JS, so without this the very first frame's settings/highscore load
// would race the load and always see an empty filesystem.
volatile bool idbfsSyncPending = false;
} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE void onIdbfsSynced() { idbfsSyncPending = false; }

void platformInitSaveData()
{
    idbfsSyncPending = true;
    // clang-format off
    EM_ASM(
        FS.mkdir('/save');
        FS.mount(IDBFS, {}, '/save');
        FS.syncfs(true, function (err) {
            if (err) { console.error('Galaxy Impact: IDBFS load failed', err); }
            _onIdbfsSynced();
        });
    );
    // clang-format on
    while (idbfsSyncPending)
    {
        emscripten_sleep(10);
    }
}

void platformSyncSaveData()
{
    idbfsSyncPending = true;
    // clang-format off
    EM_ASM(
        FS.syncfs(false, function (err) {
            if (err) { console.error('Galaxy Impact: IDBFS save failed', err); }
            _onIdbfsSynced();
        });
    );
    // clang-format on
    while (idbfsSyncPending)
    {
        emscripten_sleep(10);
    }
}

#else

auto getSaveDataDir() -> std::string
{
    std::filesystem::path dir;

#if defined(_WIN32)
    if (const char* appData = std::getenv("APPDATA"); appData != nullptr)
    {
        dir = std::filesystem::path(appData) / "GalaxyImpact";
    }
#elif defined(__APPLE__)
    if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        dir = std::filesystem::path(home) / "Library" / "Application Support" / "GalaxyImpact";
    }
#else // Linux and other POSIX desktops (also covers the Steam Deck)
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && *xdg != '\0')
    {
        dir = std::filesystem::path(xdg) / "GalaxyImpact";
    }
    else if (const char* home = std::getenv("HOME"); home != nullptr)
    {
        dir = std::filesystem::path(home) / ".local" / "share" / "GalaxyImpact";
    }
#endif

    if (dir.empty())
    {
        // No environment to resolve a proper user-data dir from (unusual,
        // but shouldn't crash) - fall back to the working directory, the
        // old behavior.
        return "";
    }

    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    return dir.string() + "/";
}

void platformInitSaveData() {}
void platformSyncSaveData() {}

#endif
