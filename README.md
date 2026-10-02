# Yozora Browser

**A native desktop browser built with C++ and Qt WebEngine.**

Yozora (夜空 — "night sky") is a small, fast desktop web browser for Windows and
Linux. It is a real C++ desktop application with its own interface, not a web
site in a wrapper and not an Electron app. Pages are rendered by Chromium
through Qt WebEngine.

> **Status: 0.2.7.** Windows installers are published on the
> [releases page](https://github.com/Stuqpidb/yozora-browser/releases). Linux is
> not packaged yet. The interface is still moving; see the roadmap below.

---

## What works today

- Tabs: create, close, reorder, restore last closed (`Ctrl+Shift+T`), duplicate
  and pin (right-click a tab)
- Navigation: back, forward, reload, stop, with buttons that disable themselves
- Address bar: URL or search query, `Ctrl+L` to focus; `Ctrl+wheel` zooms with an
  on-screen percentage
- A left rail that can be hidden (`Ctrl+B`) and stays that way across restarts
- Yozora start page: search over a generated night sky, with pinned sites
- History (`Ctrl+H`) and bookmarks (`Ctrl+Shift+O`) as searchable lists, with
  relative times and per-entry removal
- Downloads window with per-file progress, open, and show-in-folder
- Session restore on start, and recovery of the open tabs after a crash
- Optional background update checks (off by default; see the privacy notes)
- Page context menu: open link, copy link, save image, clipboard actions
- DevTools in a separate window (`F12`)
- Downloads to a configurable folder, with progress and "open" / "show folder"
- Yozora-branded error pages
- Settings: search engine (including a custom one), home page, download folder,
  scroll behaviour

### Privacy & security

- **No telemetry, no tracking, no account, no cloud.**
- Third-party cookies blocked by default; cookies can be made session-only.
- Built-in ad and tracker blocking (in the spirit of Brave) with a per-page
  shield in the address bar and a per-site allowlist, plus an optional local
  blocklist. No extension required.
- Optional `Do Not Track` / `Global Privacy Control` headers.
- Per-site permissions (camera, microphone, location, notifications, clipboard,
  fonts, pointer lock) that default to **Ask**; screen sharing is refused.
- Private browsing windows backed by a separate off-the-record profile.
- Clear browsing data (cookies, cache, visited links, permissions).
- Download names are sanitised, files are never overwritten, and programs are
  never run automatically.
- `file://` links and external protocols (`mailto:`, `tel:`, `magnet:`, ...)
  are never opened from web content without an explicit prompt.
- Certificate errors are always rejected; there is no "proceed anyway".

See [docs/PRIVACY.md](docs/PRIVACY.md) and [docs/SECURITY.md](docs/SECURITY.md)
for the details and the honest limits (Yozora does **not** hide your IP).

## Not in the MVP

Deliberately absent for now: accounts, sync, a server, extensions, a password
manager, history and bookmarks managers, telemetry, ads, mobile. See the roadmap
below.

## Technology

| | |
|---|---|
| Language | C++20 |
| UI | Qt 6 Widgets (6.8+; developed against 6.8.3 LTS) |
| Engine | Qt WebEngine (Chromium) |
| Build | CMake + Ninja or MSVC |
| Platform | Windows 10/11 (primary), Linux (buildable) |

Qt WebEngine is treated as a replaceable backend: all engine access is confined
to `src/web/`, so swapping in a different engine later touches one directory.

## Building

### Requirements

- CMake 3.21+
- A C++20 compiler (MSVC 2022 / Clang 15+ / GCC 11+)
- Qt 6.8 or newer with the **WebEngine** module (the per-origin permission API
  arrives in 6.8)
- Ninja (optional, but much faster than MSVC)

### Windows

```bat
scripts\build.cmd
```

This finds Visual Studio Build Tools, sets up the MSVC environment, configures
with CMake and builds. The binary lands in `build\Release\bin\Yozora.exe`.

Other targets:

```bat
scripts\build.cmd debug     :: Debug build
scripts\build.cmd test      :: build, then run the unit tests
scripts\build.cmd install   :: build, then stage a runnable folder
scripts\build.cmd package   :: build YozoraSetup-<version>.exe
```

Set `QT_ROOT` if your Qt lives somewhere else:

```bat
set QT_ROOT=C:\Qt\6.8.3\msvc2022_64
scripts\build.cmd
```

### Linux

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64cmake --build build
./build/bin/Yozora
```

Dependencies on Debian/Ubuntu:

```sh
sudo apt install build-essential cmake ninja-build \
    qt6-base-dev qt6-webengine-dev
```

## Project layout

```
src/
  main.cpp             entry point; wiring only, no browser logic
  app/                 filesystem locations
  browser/             main window and tab model
  core/                settings, search engines, theme, night sky, update check
  privacy/             tracker list, request interceptor, permissions, download safety
  home/                the start page: search field and pinned sites
  ui/                  address bar, navigation bar, tab strip, dialogs
  utils/               URL parsing and search detection
  web/                 everything that touches Qt WebEngine
resources/             icons, bundled fonts, the tracker list, the Qt resource bundle
docs/                  privacy and security notes
tests/                 unit tests (URL logic, tab behaviour, privacy, painting)
cmake/                 packaging rules
installer/             installer assets and the font / icon generators
```

The dependency direction is one-way: `browser/` knows about `ui/`, `web/` and
`privacy/`; `ui/` knows about `core/`; `web/` knows about `core/`, `utils/` and
`privacy/`. Nothing in `web/` knows about windows, and nothing in `ui/` knows
about WebEngine (the profile exposes a WebEngine-free view to the settings).

## Where data is stored

Everything stays on the machine, under the platform's application-data
directory:

- Windows: `%LOCALAPPDATA%\Yozora\Yozora Browser`
- Linux: `~/.local/share/Yozora/Yozora Browser`

The subdirectory `profile/` holds the Chromium profile — cookies, localStorage,
cache, service workers. Deleting it resets the browser to a clean state. An
optional `privacy/blocklist.txt` lets you extend the tracker list locally.

`state/` holds small JSON files that belong to the user rather than to Chromium:
`bookmarks.json`, `history.json` and `pinned-sites.json` (the sites pinned to
the start page).

## Roadmap

MVP first (done). Privacy hardening (Phase 2) is in place: cookies, ad and
tracker blocking, permissions, private windows and download safety. Next, in no
particular order: history, bookmarks, a real download manager, browser
profiles, WebRTC/fingerprinting hardening where the engine allows it, DNS over
HTTPS, extensions, and auto-update. A full community-list ad blocker is
explicitly not a goal; the bundled curated lists are.

## License

MIT for the code. See [LICENSE](LICENSE).

The build redistributes Qt, which is licensed separately; Yozora takes the
LGPLv3 option. The licence texts of everything that ships with the application,
plus a note on how to replace the Qt libraries, are in
[LICENSES/](LICENSES/) and are installed into the `licenses` folder next to the
executable.

The bundled typefaces are third-party and carry their own licence:
[Inter](https://github.com/rsms/inter) and
[Space Grotesk](https://fonts.google.com/specimen/Space+Grotesk) are both under
the SIL Open Font License 1.1, each with its own text in `LICENSES/`. They are
rebuilt from the upstream variable fonts by `installer/make_fonts.py`.
