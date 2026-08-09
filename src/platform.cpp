#include "platform.hpp"

#include <cstdlib>
#include <filesystem>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

auto getSaveDataDir() -> std::string { return "/save/"; }

namespace
{

volatile bool idbfsSyncPending = false;
}

extern "C" EMSCRIPTEN_KEEPALIVE void onIdbfsSynced() { idbfsSyncPending = false; }

void platformInitSaveData()
{
    idbfsSyncPending = true;

    EM_ASM(FS.mkdir('/save'); FS.mount(IDBFS, {}, '/save'); FS.syncfs(
        true, function(err) {
            if (err)
            {
                console.error('Galaxy Impact: IDBFS load failed', err);
            }
            _onIdbfsSynced();
        }););
}

auto platformSaveDataReady() -> bool { return !idbfsSyncPending; }

void platformSyncSaveData()
{

    EM_ASM(FS.syncfs(
        false, function(err) {
            if (err)
            {
                console.error('Galaxy Impact: IDBFS save failed', err);
            }
        }););
}

void platformExitToLanding() { EM_ASM(window.location.href = 'index.html';); }

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
#else
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

        return "";
    }

    std::error_code error_code;
    std::filesystem::create_directories(dir, error_code);
    return dir.string() + "/";
}

void platformInitSaveData() {}
auto platformSaveDataReady() -> bool { return true; }
void platformSyncSaveData() {}
void platformExitToLanding() {}

#endif
