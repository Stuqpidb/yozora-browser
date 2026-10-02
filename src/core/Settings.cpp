// SPDX-License-Identifier: MIT
#include "core/Settings.h"

#include "core/SearchEngine.h"
#include "utils/Version.h"

#include <QDir>
#include <QStandardPaths>
#include <QSettings>

namespace yozora {

class SettingsPrivate {
public:
    QSettings store;
};

namespace {
constexpr auto kKeySearchEngine = "browser/search_engine";
constexpr auto kKeyCustomSearchName = "browser/custom_search_name";
constexpr auto kKeyCustomSearchUrl = "browser/custom_search_url";
constexpr auto kKeyHomePage = "browser/home_page";
constexpr auto kKeyDownloadDir = "downloads/directory";
constexpr auto kKeyAskWhereToSave = "downloads/ask_where_to_save";
constexpr auto kKeyScrollMode = "appearance/scroll_mode";
constexpr auto kKeyWindowGeometry = "window/geometry";
constexpr auto kKeyWindowState = "window/state";
constexpr auto kKeySideBarCollapsed = "window/sidebar_collapsed";
constexpr auto kKeyRestoreSession = "session/restore_on_start";
constexpr auto kKeyBackgroundUpdates = "updates/background";
constexpr auto kKeyHardwareAcceleration = "appearance/hardware_acceleration";

constexpr auto kKeyBlockThirdPartyCookies = "privacy/block_third_party_cookies";
constexpr auto kKeyKeepCookiesOnExit = "privacy/keep_cookies_on_exit";
constexpr auto kKeyBlockAds = "privacy/block_ads";
constexpr auto kKeyAdBlockAllowlist = "privacy/adblock_allowlist";
constexpr auto kKeySendDnt = "privacy/send_do_not_track";
constexpr auto kKeyNotifications = "privacy/notifications_enabled";
constexpr auto kKeyWebRtc = "privacy/webrtc_policy";
}  // namespace

Settings::Settings(QObject* parent)
    : QObject(parent)
    , d(new SettingsPrivate)
{
}

Settings::~Settings() = default;

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

QString Settings::searchEngineId() const
{
    const QString fallback = SearchEngines::defaultId();
    const QString id = d->store.value(QLatin1String(kKeySearchEngine), fallback).toString();
    // A custom engine is always valid, even before it has been configured:
    // the user may be halfway through filling it in.
    if (id == QLatin1String(SearchEngines::kCustomId)) {
        return id;
    }
    // Guard against a stale id after an update removed the engine.
    return SearchEngines::byId(id).id;
}

void Settings::setSearchEngineId(const QString& id)
{
    if (id.isEmpty() || searchEngineId() == id) {
        return;
    }
    d->store.setValue(QLatin1String(kKeySearchEngine), id);
    emit searchEngineChanged();
}

SearchEngine Settings::searchEngine() const
{
    const QString id = searchEngineId();
    if (id == QLatin1String(SearchEngines::kCustomId)) {
        const SearchEngine engine = SearchEngines::custom(customSearchEngineName(),
                                                          customSearchEngineUrl());
        // An unusable custom engine must not leave the address bar dead: fall
        // back to the default engine until it is filled in properly.
        if (!engine.queryUrl.isEmpty()) {
            return engine;
        }
        return SearchEngines::byId(SearchEngines::defaultId());
    }
    return SearchEngines::byId(id);
}

QString Settings::customSearchEngineName() const
{
    return d->store.value(QLatin1String(kKeyCustomSearchName)).toString();
}

void Settings::setCustomSearchEngineName(const QString& name)
{
    if (customSearchEngineName() == name) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyCustomSearchName), name);
    emit searchEngineChanged();
}

QString Settings::customSearchEngineUrl() const
{
    return d->store.value(QLatin1String(kKeyCustomSearchUrl)).toString();
}

void Settings::setCustomSearchEngineUrl(const QString& url)
{
    if (customSearchEngineUrl() == url) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyCustomSearchUrl), url);
    emit searchEngineChanged();
}

// ---------------------------------------------------------------------------
// General
// ---------------------------------------------------------------------------

QString Settings::homePage() const
{
    return d->store.value(QLatin1String(kKeyHomePage), QStringLiteral("about:yozora")).toString();
}

void Settings::setHomePage(const QString& url)
{
    if (url.isEmpty() || homePage() == url) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyHomePage), url);
    emit homePageChanged();
}

QString Settings::downloadDirectory() const
{
    const QString stored = d->store.value(QLatin1String(kKeyDownloadDir)).toString();
    if (!stored.isEmpty() && QDir(stored).exists()) {
        return stored;
    }
    const QString fallback =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!fallback.isEmpty()) {
        return fallback;
    }
    return QDir::homePath();
}

void Settings::setDownloadDirectory(const QString& path)
{
    if (path.isEmpty() || downloadDirectory() == path) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyDownloadDir), path);
    emit downloadDirectoryChanged();
}

bool Settings::askWhereToSave() const
{
    return d->store.value(QLatin1String(kKeyAskWhereToSave), false).toBool();
}

void Settings::setAskWhereToSave(bool ask)
{
    d->store.setValue(QLatin1String(kKeyAskWhereToSave), ask);
}

QByteArray Settings::windowGeometry() const
{
    return d->store.value(QLatin1String(kKeyWindowGeometry)).toByteArray();
}

void Settings::setWindowGeometry(const QByteArray& geometry)
{
    d->store.setValue(QLatin1String(kKeyWindowGeometry), geometry);
}

Settings::ScrollMode Settings::scrollMode() const
{
    const int raw = d->store.value(QLatin1String(kKeyScrollMode),
                                   static_cast<int>(ScrollMode::Fast))
                        .toInt();
    if (raw < 0 || raw > 2) {
        return ScrollMode::Fast;
    }
    return static_cast<ScrollMode>(raw);
}

void Settings::setScrollMode(ScrollMode mode)
{
    if (scrollMode() == mode) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyScrollMode), static_cast<int>(mode));
    emit scrollModeChanged();
}

QByteArray Settings::windowState() const
{
    return d->store.value(QLatin1String(kKeyWindowState)).toByteArray();
}

void Settings::setWindowState(const QByteArray& state)
{
    d->store.setValue(QLatin1String(kKeyWindowState), state);
}

bool Settings::sideBarCollapsed() const
{
    return d->store.value(QLatin1String(kKeySideBarCollapsed), false).toBool();
}

void Settings::setSideBarCollapsed(bool collapsed)
{
    d->store.setValue(QLatin1String(kKeySideBarCollapsed), collapsed);
}

bool Settings::restoreSessionOnStart() const
{
    return d->store.value(QLatin1String(kKeyRestoreSession), true).toBool();
}

void Settings::setRestoreSessionOnStart(bool restore)
{
    d->store.setValue(QLatin1String(kKeyRestoreSession), restore);
}

bool Settings::backgroundUpdates() const
{
    // Opt-in on purpose: the privacy notes promise no unsolicited network
    // traffic, so this has to be turned on deliberately.
    return d->store.value(QLatin1String(kKeyBackgroundUpdates), false).toBool();
}

void Settings::setBackgroundUpdates(bool enabled)
{
    if (backgroundUpdates() == enabled) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyBackgroundUpdates), enabled);
    emit backgroundUpdatesChanged();
}

bool Settings::hardwareAcceleration() const
{
    return d->store.value(QLatin1String(kKeyHardwareAcceleration), true).toBool();
}

void Settings::setHardwareAcceleration(bool enabled)
{
    if (hardwareAcceleration() == enabled) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyHardwareAcceleration), enabled);
    emit hardwareAccelerationChanged();
}

bool Settings::bootHardwareAcceleration()
{
    // Read before QApplication exists, like the WebRTC policy, because the
    // decision has to reach Chromium at process start through an environment
    // variable.
    const QSettings store(QSettings::NativeFormat, QSettings::UserScope,
                          QString::fromLatin1(kOrganization),
                          QString::fromLatin1(kDisplayName));
    return store.value(QLatin1String(kKeyHardwareAcceleration), true).toBool();
}

// ---------------------------------------------------------------------------
// Privacy
// ---------------------------------------------------------------------------

bool Settings::blockThirdPartyCookies() const
{
    return d->store.value(QLatin1String(kKeyBlockThirdPartyCookies), true).toBool();
}

void Settings::setBlockThirdPartyCookies(bool block)
{
    if (blockThirdPartyCookies() == block) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyBlockThirdPartyCookies), block);
    emit cookiePolicyChanged();
}

bool Settings::keepCookiesOnExit() const
{
    return d->store.value(QLatin1String(kKeyKeepCookiesOnExit), true).toBool();
}

void Settings::setKeepCookiesOnExit(bool keep)
{
    if (keepCookiesOnExit() == keep) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyKeepCookiesOnExit), keep);
    emit cookiePolicyChanged();
}

bool Settings::blockAds() const
{
    // The old key is still honoured so an existing installation keeps its
    // choice after the rename from "trackers" to "ads"; new writes use the new
    // key.
    if (d->store.contains(QLatin1String(kKeyBlockAds))) {
        return d->store.value(QLatin1String(kKeyBlockAds), true).toBool();
    }
    return d->store.value(QStringLiteral("privacy/block_trackers"), true).toBool();
}

void Settings::setBlockAds(bool block)
{
    if (blockAds() == block) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyBlockAds), block);
    emit adBlockingChanged();
}

QStringList Settings::adBlockAllowlist() const
{
    return d->store.value(QLatin1String(kKeyAdBlockAllowlist)).toStringList();
}

void Settings::allowSiteForAdBlock(const QString& host)
{
    const QString normalized = host.toLower().trimmed();
    if (normalized.isEmpty()) {
        return;
    }
    QStringList list = adBlockAllowlist();
    if (list.contains(normalized)) {
        return;
    }
    list.append(normalized);
    d->store.setValue(QLatin1String(kKeyAdBlockAllowlist), list);
    emit adBlockingChanged();
}

void Settings::disallowSiteForAdBlock(const QString& host)
{
    const QString normalized = host.toLower().trimmed();
    QStringList list = adBlockAllowlist();
    if (list.removeAll(normalized) == 0) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyAdBlockAllowlist), list);
    emit adBlockingChanged();
}

bool Settings::isSiteAllowedForAdBlock(const QString& host) const
{
    return adBlockAllowlist().contains(host.toLower().trimmed());
}

bool Settings::sendDoNotTrack() const
{
    return d->store.value(QLatin1String(kKeySendDnt), true).toBool();
}

void Settings::setSendDoNotTrack(bool send)
{
    if (sendDoNotTrack() == send) {
        return;
    }
    d->store.setValue(QLatin1String(kKeySendDnt), send);
    emit doNotTrackChanged();
}

bool Settings::notificationsEnabled() const
{
    return d->store.value(QLatin1String(kKeyNotifications), true).toBool();
}

void Settings::setNotificationsEnabled(bool enabled)
{
    if (notificationsEnabled() == enabled) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyNotifications), enabled);
    emit notificationsEnabledChanged();
}

Settings::WebRtcPolicy Settings::webrtcPolicy() const
{
    const int raw = d->store.value(QLatin1String(kKeyWebRtc),
                                   static_cast<int>(WebRtcPolicy::Default))
                        .toInt();
    if (raw < 0 || raw > 2) {
        return WebRtcPolicy::Default;
    }
    return static_cast<WebRtcPolicy>(raw);
}

void Settings::setWebRtcPolicy(WebRtcPolicy policy)
{
    if (webrtcPolicy() == policy) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyWebRtc), static_cast<int>(policy));
    emit webrtcPolicyChanged();
}

Settings::WebRtcPolicy Settings::bootWebRtcPolicy()
{
    // This runs before QApplication is constructed, so the application name and
    // organisation are not set yet and the usual Settings object cannot be
    // used. The scope is spelled out explicitly to read the same store the rest
    // of the application writes to.
    const QSettings store(QSettings::NativeFormat, QSettings::UserScope,
                          QString::fromLatin1(kOrganization),
                          QString::fromLatin1(kDisplayName));
    const int raw = store.value(QLatin1String(kKeyWebRtc),
                                static_cast<int>(WebRtcPolicy::Default))
                        .toInt();
    if (raw < 0 || raw > 2) {
        return WebRtcPolicy::Default;
    }
    return static_cast<WebRtcPolicy>(raw);
}

void Settings::resetToDefaults()
{
    d->store.clear();
    emit searchEngineChanged();
    emit homePageChanged();
    emit downloadDirectoryChanged();
    emit scrollModeChanged();
    emit cookiePolicyChanged();
    emit adBlockingChanged();
    emit doNotTrackChanged();
    emit notificationsEnabledChanged();
    emit webrtcPolicyChanged();
    emit backgroundUpdatesChanged();
    emit hardwareAccelerationChanged();
}

}  // namespace yozora
