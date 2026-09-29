# Yozora Browser

**A native desktop browser built with C++ and Qt WebEngine.**

Yozora (夜空 — "night sky") is a small, fast desktop web browser for Windows and
Linux. It is a real C++ desktop application with its own interface, not a web
site in a wrapper and not an Electron app. Pages are rendered by Chromium
through Qt WebEngine.

> **Status: private development.** The MVP is a work in progress. Nothing here
> is released publicly yet, and the project must not be pushed to a public
> repository.

---

## What works today

- Tabs: create, close, reorder, restore last closed (`Ctrl+Shift+T`)
- Navigation: back, forward, reload, stop, with buttons that disable themselves
- Address bar: URL or search query, `Ctrl+L` to focus
- Yozora start page with a night-sky background
- Page context menu: open link, copy link, save image, clipboard actions
- DevTools in a separate window (`F12`)
- Downloads to a configurable folder, with progress and "open" / "show folder"
- Yozora-branded error pages
- Settings: search engine, new tab page, download folder, dark / light theme
- All browsing data stored locally; cookies and logins survive a restart

## Not in the MVP

Deliberately absent for now: accounts, sync, a server, extensions, an ad
blocker, a password manager, history and bookmarks managers, telemetry, ads,
mobile. See the roadmap below.

## Technology

| | |
|---|---|
| Language | C++20 |
| UI | Qt 6 Widgets (6.5+; developed against 6.8.3 LTS) |
| Engine | Qt WebEngine (Chromium) |
| Build | CMake + Ninja or MSVC |
| Platform | Windows 10/11 (primary), Linux (buildable) |

Qt WebEngine is treated as a replaceable backend: all engine access is confined
to `src/web/`, so swapping in a different engine later touches one directory.

## Building

### Requirements

- CMake 3.21+
- A C++20 compiler (MSVC 2022 / Clang 15+ / GCC 11+)
- Qt 6.5 or newer with the **WebEngine** module
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
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64
cmake --build build
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
  core/                settings, search engines, theme, update check
  ui/                  address bar, navigation bar, tab strip, start page, settings
  utils/               URL parsing and search detection
  web/                 everything that touches Qt WebEngine
resources/             icons and the Qt resource bundle
tests/                 unit tests (URL logic, tab behaviour)
cmake/                 packaging rules
installer/             installer assets
```

The dependency direction is one-way: `browser/` knows about `ui/` and `web/`,
`ui/` knows about `core/`, and `web/` knows about `core/` and `utils/`. Nothing
in `web/` knows about windows, and nothing in `ui/` knows about WebEngine.

## Where data is stored

Everything stays on the machine, under the platform's application-data
directory:

- Windows: `%LOCALAPPDATA%\Yozora\Yozora Browser`
- Linux: `~/.local/share/Yozora/Yozora Browser`

The subdirectory `profile/` holds the Chromium profile — cookies, localStorage,
cache, service workers. Deleting it resets the browser to a clean state.

## Roadmap

MVP first, then, in order: history, bookmarks, a real download manager,
profiles, extensions, ad blocking, privacy features, auto-update.

## License

MIT. See [LICENSE](LICENSE).
