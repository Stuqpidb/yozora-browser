# Yozora security notes

Yozora is a native Qt/C++ application that renders pages with Qt WebEngine
(Chromium). This document describes the security decisions and the rules the
code follows. It is meant to be read before changing anything in `src/web/`,
`src/privacy/` or `src/browser/`.

## Do not weaken Chromium

Yozora never disables a Chromium security mechanism to make a feature work:

- the sandbox stays on; `--no-sandbox` is never passed,
- same-origin policy, origin isolation and site isolation are untouched,
- certificate verification is never bypassed,
- mixed-content protection is not relaxed,
- insecure-content flags are not used.

If a feature would require any of the above, the feature changes, not the
security model.

## Tabs and processes

Each tab runs in Chromium's normal renderer process with the standard sandbox.
Yozora does not merge renderers, does not add renderer privileges, and does not
load arbitrary code into the renderer beyond the pages themselves.

## Internal pages and the absence of a bridge

Yozora's own surfaces are **native Qt widgets**, not web pages:

- the home page (`HomePage`),
- the settings dialog,
- the permission prompt,
- the error page (generated as local HTML with the `yozora-error://` base).

Consequences:

- There is **no `QWebChannel`** and **no JS ↔ C++ bridge** in the process. A web
  page has no object, function or message path into native code. There is
  nothing to validate because nothing is exposed.
- There is **no privileged `yozora://` scheme handler**. Internal pages cannot
  be navigated to from web content in a way that grants extra powers, and web
  content cannot reach them for privileged behaviour.
- Error pages are built by string concatenation from a trusted template, with
  the only dynamic parts (host, network error text) HTML-escaped.

If a bridge is ever added, it must be: untrusted web → strict validation →
small, explicit API → native code, and it must be security-reviewed on its own.

## Navigation policy (`WebPage::acceptNavigationRequest`)

Every navigation goes through one gate:

- **Allowed:** the schemes the engine renders itself (`http`, `https`, `ws`,
  `wss`, `ftp`, `about`, `data`, `blob`, `qrc`, `yozora-error`, `javascript`,
  `view-source`).
- **`file:`** is allowed **only** when the user typed it (or navigated back to
  it) in the main frame. A page that links to, redirects to, or frames a
  `file:` URL is refused, so web content cannot read local files.
- **Everything else** (`mailto:`, `tel:`, `magnet:`, unknown custom schemes) is
  never launched silently. The window asks first and then hands it to the
  operating system via the desktop services.

## Certificates and HTTPS

- Certificate errors are never ignored. `acceptCertificate()` is not called
  anywhere in the code base.
- When a certificate fails, the load is rejected and Yozora's own error page
  explains the specific problem (expired, wrong host, untrusted authority,
  revoked, pinned-key change, ...).
- There is no "proceed anyway" button.

## Permissions

Permissions are handled by Chromium's per-origin store through
`QWebEnginePermission`:

- the default is to ask,
- grants are per origin and never global,
- screen sharing is always refused (no picker exists yet),
- weak or non-HTTP(S) origins (internal pages) can never be granted anything,
- the user can review and revoke every stored permission in Settings.

## Downloads

- A server-supplied filename is sanitised before it touches the filesystem:
  path separators, `..`, control characters, Windows device names and illegal
  characters are removed; the result is length-limited.
- Existing files are never overwritten; a free name is chosen instead.
- Files that can execute code or install software (`.exe`, `.msi`, `.bat`,
  `.ps1`, `.vbs`, `.hta`, `.jar`, ...) are **never** run automatically. Even
  opening one by hand goes through an explicit warning.
- "Ask where to save" can be enabled so the destination is chosen per file.

## Local files

`file://` pages cannot reach remote content, and remote pages cannot reach
`file://` content: the WebEngine settings `LocalContentCanAccessRemoteUrls` and
`LocalContentCanAccessFileUrls` are both off, and the navigation gate above
blocks `file:` links from web content.

## Ad and tracker blocking

The request interceptor blocks requests that match the bundled ad and tracker
filter lists and can attach `DNT` / `Sec-GPC` headers. It runs on the Chromium
IO thread and therefore only reads immutable snapshots and atomics; it never
touches UI or `Settings` objects directly. Matching is limited to the request
URL, host, resource type and whether the request is third party. Blocking never
touches the user's own navigations, and a site can be allowed by exact host
through the shield.

The rule engine is deliberately small and fails closed: any syntax it does not
understand is ignored rather than guessed at, so a rule can never be applied
more broadly than it was written. The lists are compiled into the binary as
resources and are never downloaded at run time.

## Private browsing isolation

Private windows use a separate off-the-record profile. They cannot read the
persistent profile's cookies or storage and leave nothing behind. The private
profile is not shared with, and does not leak into, the normal profile.

## Known limits

- **No JS ↔ C++ bridge exists, so none is audited**; this changes the moment a
  bridge is added.
- **Remote debugging** (DevTools) is available to the user (F12) but is not
  exposed to the network by default.
- Yozora relies on Qt WebEngine and Chromium for the vast majority of its
  attack surface; keeping Qt patched is part of keeping Yozora secure.
