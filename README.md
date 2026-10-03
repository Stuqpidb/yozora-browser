# Yozora Browser

A small, fast desktop web browser written in C++ with Qt WebEngine. Yozora
(夜空 — "night sky") is a native desktop application with its own interface, not
a web page in a wrapper and not an Electron app. Pages are rendered by Chromium
through Qt WebEngine.

> **Status: 0.4.0.** Windows installers are on the
> [releases page](https://github.com/Stuqpidb/yozora-browser/releases). Linux is
> not packaged; it builds from source.

## Features

- **Tabs** — create, close, reorder, restore the last closed (`Ctrl+Shift+T`),
  duplicate and pin (right-click a tab). Tabs animate in and out.
- **Chrome-style window** — no system title bar: the tabs and the
  minimise/maximise/close buttons share the top row, the empty strip drags the
  window (with Windows snap), the edges and corners resize it, and `F11` is full
  screen. A page's video can also go full screen.
- **Navigation** — back, forward, reload, stop.
- **Address bar** — a URL or a search query, `Ctrl+L` to focus; `Ctrl+wheel`
  zooms with an on-screen percentage.
- **Start page** — a generated night sky with a search field and your pinned
  sites. Nothing is pinned for a new install.
- **History** (`Ctrl+H`) and **bookmarks** (`Ctrl+Shift+O`) as searchable lists
  with relative times and per-entry removal.
- **Downloads** window with per-file progress, open, and show in folder.
- **Session restore** on start, and recovery of the open tabs after a crash.
- **Sidebar** — a left rail for quick access; hide it with `Ctrl+B` or turn it
  off entirely in Settings → Interface.
- Page context menu, and DevTools in a separate window (`F12`).
- Settings for search engine (including a custom one), home page, downloads,
  privacy, interface, scrolling and data.

## Privacy & security

- **No telemetry, no tracking, no account, no cloud.**
- **Built-in ad and tracker blocking** in the spirit of Brave: curated lists
  ship with the browser, no extension and nothing downloaded at run time. A
  shield in the address bar shows what was blocked on the page and lets a single
  site be allowed.
- Third-party cookies are blocked by default; cookies can be made session-only.
- Per-site permissions (camera, microphone, location, notifications, clipboard,
  fonts, pointer lock) default to **Ask**; screen sharing is refused.
- Private windows use a separate off-the-record profile.
- Clear browsing data (cookies, cache, visited links, permissions).
- Downloads are sanitised, files are never overwritten, and programs are never
  run automatically.
- `file://` links and external protocols (`mailto:`, `tel:`, `magnet:`, ...) are
  never opened from web content without a prompt.
- Certificate errors are always rejected — there is no "proceed anyway".
- Background update checks are off until you turn them on.

See [docs/PRIVACY.md](docs/PRIVACY.md) and [docs/SECURITY.md](docs/SECURITY.md)
for the details and the honest limits. Yozora does **not** hide your IP address.

## Not included

By design, for now: accounts, sync, a server, extensions, a password manager, a
full community ad-block list, mobile, and a packaged Linux build.

## Technology

| | |
|---|---|
| Language | C++20 |
| UI | Qt 6 Widgets (6.8+) |
| Engine | Qt WebEngine (Chromium) |
| Build | CMake + Ninja or MSVC |
| Platform | Windows 10/11 (packaged); Linux (build from source) |

All engine access is confined to `src/web/`, so the rendering backend is
replaceable in one directory.

## Building

**Windows**

```bat
scripts\build.cmd
```

It locates the MSVC toolchain, configures with CMake and builds. The binary
lands in `build\Release\bin\Yozora.exe`. Other targets: `debug`, `test`,
`install`, `package`. Set `QT_ROOT` if Qt is somewhere other than
`C:\Qt\6.8.3\msvc2022_64`.

**Linux**

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH=/path/to/Qt/6.8.3/gcc_64
cmake --build build
./build/bin/Yozora
```

Requirements: CMake 3.21+, a C++20 compiler, and Qt 6.8+ with the WebEngine
module. On Debian/Ubuntu: `qt6-base-dev qt6-webengine-dev` (plus CMake and
Ninja).

## Where data is stored

Everything stays on the machine, under the platform application-data directory:

- Windows: `%LOCALAPPDATA%\Yozora\Yozora Browser`
- Linux: `~/.local/share/Yozora/Yozora Browser`

`profile/` is the Chromium profile (cookies, localStorage, cache, service
workers); deleting it resets the browser. `state/` holds the small files that
belong to you rather than to Chromium: `bookmarks.json`, `history.json`,
`pinned-sites.json` and `session.json`. `privacy/blocklist.txt` lets you add
your own filter rules locally.

## License

MIT for the code — see [LICENSE](LICENSE). Qt is redistributed under the LGPLv3;
its licence texts, along with those of the bundled fonts
([Inter](https://github.com/rsms/inter) and
[Space Grotesk](https://fonts.google.com/specimen/Space+Grotesk), both SIL OFL
1.1), are in [LICENSES/](LICENSES/).
