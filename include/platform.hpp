#pragma once

#include <string>

// getSaveDataDir returns a writable, OS-appropriate directory (created if
// missing) for persisted files (settings.txt, highscore.txt), trailing slash
// included - callers just append a filename. Desktop: the per-user app-data
// directory (%APPDATA%, XDG_DATA_HOME/~/.local/share, or
// ~/Library/Application Support), NOT the executable's own directory - Steam
// can mark the install directory read-only or wipe it on verify-integrity,
// and installers/updates in general shouldn't be assumed writable. Web
// (Emscripten): a fixed virtual path mounted to IndexedDB by
// platformInitSaveData, since there's no real filesystem otherwise.
auto getSaveDataDir() -> std::string;

// platformInitSaveData does whatever one-time setup a platform needs before
// any file at getSaveDataDir() can be read - a no-op everywhere except Web,
// where it mounts IDBFS and blocks (via Asyncify) until the previous
// session's save data has finished loading from IndexedDB. Call once, before
// the first settings/highscore load.
void platformInitSaveData();

// platformSyncSaveData flushes any writes made since the last call back to
// persistent storage - a no-op everywhere except Web, where writes to the
// mounted IDBFS path are only an in-memory mirror until explicitly synced to
// IndexedDB. Call after every settings/highscore save.
void platformSyncSaveData();
