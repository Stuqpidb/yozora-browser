// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

namespace yozora {

// Central place for every filesystem location Yozora uses. Keeping it here
// means the on-disk layout can change without touching call sites, and makes
// it obvious what is user data (and therefore must never be committed).
class AppPaths {
public:
    // Root of the persistent user data directory, e.g.
    // %LOCALAPPDATA%\Yozora\Yozora Browser
    [[nodiscard]] static QString userDataDir();

    // Chromium profile directory handed to QWebEngineProfile (cookies,
    // localStorage, cache, service workers).
    [[nodiscard]] static QString profileDir();

    // Where crash dumps and logs end up.
    [[nodiscard]] static QString logsDir();

    // Local privacy data, currently just the optional user tracker blocklist.
    [[nodiscard]] static QString privacyDir();

    // Optional, user supplied tracker blocklist (same format as the bundled
    // one). Never uploaded; it only extends the built-in list on this machine.
    [[nodiscard]] static QString trackerListPath();

    // Local, user owned state that is not Chromium data: bookmarks, history and
    // the pinned sites on the start page. All plain JSON on this machine.
    [[nodiscard]] static QString stateDir();
    [[nodiscard]] static QString bookmarksPath();
    [[nodiscard]] static QString historyPath();
    [[nodiscard]] static QString pinnedSitesPath();

    // The start page used to be a board of movable widgets whose positions were
    // saved to home.json. It is a fixed layout now; the file is only read once,
    // to carry the pinned sites over, and is then left alone.
    [[nodiscard]] static QString legacyHomeLayoutPath();

    // Marker file whose presence asks the next start to wipe on-disk site
    // storage (localStorage, IndexedDB, service workers) before the profile is
    // created. Clearing those while Chromium is running is not supported, so
    // the request is deferred to the next launch instead of faked.
    [[nodiscard]] static QString siteStoragePurgeMarker();

    // Creates all directories above. Safe to call repeatedly.
    static void ensureCreated();

    // Registers the compiled qrc bundle. The bundle lives in the static core
    // library and the linker may drop its initialiser if nothing references it,
    // so this is called explicitly wherever ":/..." paths are first used.
    static void ensureResourcesLoaded();

    // Directory the application was started from - used to locate bundled
    // resources in a portable installation.
    [[nodiscard]] static QString appDir();
};

}  // namespace yozora
