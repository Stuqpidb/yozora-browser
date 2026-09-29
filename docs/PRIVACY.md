# Yozora privacy notes

Yozora is a **privacy-focused** browser. That is a design goal with concrete
rules behind it, not a promise of anonymity. This document describes what
Yozora does, what it does not do, and where the real limits are.

> **Yozora cannot hide your IP address.** No ordinary browser can. Without a
> proxy, a VPN or Tor, the sites you visit and your network operator can still
> see your IP address. Yozora will never claim otherwise.

## The rules

- No telemetry.
- No tracking.
- No advertising.
- No unnecessary network requests.
- No Yozora account.
- No cloud dependency.
- Local-first.
- Secure defaults.
- Minimal permissions.
- Minimal attack surface.

Yozora does not collect browsing data for its own servers, because it does not
have any servers of its own in the request path. The only network requests
Yozora itself makes are the ones the user explicitly triggers (see the audit
below).

## Network audit

Every network request the browser itself can make:

| Feature | Request | When |
|---|---|---|
| Startup | none | Yozora never talks to the network on launch. |
| New tab page | none | It is a native widget, not a downloaded page. |
| Search | one request, straight to the chosen engine | When the user submits a search. |
| Downloads | the file's own URL, as clicked | When the user downloads a file. |
| Error pages | none | They are generated locally as HTML. |
| Crash handling | none | There is no crash reporter. |
| Update check | GitHub Releases API | **Only** when the user picks "Check for updates". |

There is no "phone home" on startup, no background ping, no usage counter and
no installation identifier. Whatever a site itself loads is between you and
that site.

## What is stored, and where

Everything stays on the machine, under the platform application-data
directory:

- Windows: `%LOCALAPPDATA%\Yozora\Yozora Browser`
- Linux: `~/.local/share/Yozora/Yozora Browser`

| Data | Location |
|---|---|
| Cookies, localStorage, IndexedDB, service workers | `profile\` |
| HTTP cache | `profile\` |
| Settings | the platform's `QSettings` store |
| Optional tracker blocklist | `privacy\blocklist.txt` |
| Logs | `logs\` |

Deleting `profile\` resets the browser to a clean state.

## Cookies

- **Third-party cookies are blocked by default.** A cookie is dropped unless it
  belongs to the site in the address bar, which removes most cross-site
  tracking. First-party cookies still work, so logins keep working.
- "Keep cookies when Yozora closes" can be turned off, which makes every cookie
  session-only: they are gone when the browser exits.

## Tracker blocking

```
bundled filter list  ──┐
                       ├──►  URL interceptor  ──►  known tracker request blocked
local blocklist.txt  ──┘
```

- Yozora blocks requests to a curated list of well-known tracking, analytics
  and advertising **domains**. It does not try to be a full ad blocker.
- The list ships with the browser. It is never downloaded from a Yozora
  server, and nothing is uploaded.
- You can add your own rules in `privacy\blocklist.txt`. Only plain domain
  rules and `||domain^` rules are understood; anything else is ignored. The
  file is re-read on the next start.
- A direct navigation to a listed domain is allowed: blocking is for trackers
  embedded in other pages, not for where you choose to go.

## Permissions

Sites ask before they get anything sensitive. The default is to ask, per site:

| Capability | Default | Remembered? |
|---|---|---|
| Camera | Ask | No (asked every time) |
| Microphone | Ask | No |
| Location | Ask | Yes |
| Notifications | Ask | Yes |
| Clipboard read/write | Ask | Yes |
| Local fonts | Ask | Yes |
| Screen sharing | Blocked | - |
| Pointer lock | Ask | No |

- A grant is tied to one origin (`https://example.com`), never to "all sites".
- Screen sharing is refused outright: Yozora has no screen picker yet, and
  granting it without one would hand over the whole screen.
- Notifications can be switched off globally; then every notification request
  is refused without asking. (Granting notifications gates the permission, but
  Yozora does not draw OS notification pop-ups yet, so nothing is shown.)
- Your choices can be reviewed and removed in **Settings > Data**.

## What Yozora does *not* change (and why)

These are honest limitations, not oversights.

- **Fingerprinting.** Yozora does not fake canvas, WebGL, fonts, screen size,
  timezone or hardware values. Doing so consistently is very hard and usually
  breaks sites. Reducing the fingerprinting surface safely is future work, not
  a claim made today.
- **User agent.** It is left at the engine default. No spoofing.
- **WebRTC.** Yozora can limit how WebRTC shares network addresses
  (**Settings > Privacy**), but it does **not** promise to hide your IP over
  WebRTC. The option takes effect after a restart.
- **DNS.** Yozora does not run its own resolver and does not redirect DNS to
  any Yozora infrastructure. DNS-over-HTTPS, if it ever arrives, will be
  configurable by the user.
- **Do Not Track / Global Privacy Control.** These headers can be sent, but
  they are advisory: sites may ignore them. They are a small signal, not a
  shield.
- **Referrers and mixed content.** Yozora does not weaken the engine's
  defaults and does not override the referrer policy, so modern Chromium
  defaults (origin-only cross-site referrers, mixed-content blocking) apply.

## Private browsing

A private window uses a separate **off-the-record** profile:

- nothing is written to disk,
- cookies and storage are gone when the window closes,
- it shares nothing with the normal profile,
- it stores no history.

It still blocks third-party cookies and known trackers, and still asks for
permissions.

## Clear browsing data

**Settings > Data > Clear browsing data** can remove cookies, the cache,
visited-link state and stored site permissions immediately. Per-site storage
that Chromium keeps open while it runs (localStorage, IndexedDB, service
workers) is removed on the next start, because removing it live is not
supported by the engine; Yozora tells you when a restart is needed instead of
pretending it already happened. Only "All time" is offered today.
