// SPDX-License-Identifier: MIT
#pragma once

#include "core/SearchEngine.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

namespace yozora {

// Lightweight wrapper around QSettings. Every user visible preference goes
// through here so that adding a new setting later never requires touching the
// UI widgets.
//
// The defaults encoded in the getters are the privacy-focused ones: no
// telemetry exists at all, third-party cookies are blocked, known trackers are
// blocked, and a do-not-track signal is sent. Anything that weakens privacy has
// to be turned on by the user on purpose.
class Settings : public QObject {
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);
    ~Settings() override;

    // --- search -----------------------------------------------------------
    [[nodiscard]] QString searchEngineId() const;
    void setSearchEngineId(const QString& id);

    // Fully resolved engine, including the user defined custom engine.
    [[nodiscard]] SearchEngine searchEngine() const;

    [[nodiscard]] QString customSearchEngineName() const;
    void setCustomSearchEngineName(const QString& name);

    [[nodiscard]] QString customSearchEngineUrl() const;
    void setCustomSearchEngineUrl(const QString& url);

    // --- general ----------------------------------------------------------
    [[nodiscard]] QString homePage() const;
    void setHomePage(const QString& url);

    [[nodiscard]] QString downloadDirectory() const;
    void setDownloadDirectory(const QString& path);

    [[nodiscard]] bool askWhereToSave() const;
    void setAskWhereToSave(bool ask);

    // How the mouse wheel scrolls a page.
    //   Instant - the engine jumps straight to the new position.
    //   Fast    - Yozora animates the wheel with a short, snappy curve
    //             (default): smooth, but nowhere near the engine's slow curve.
    //   Smooth  - the engine's own animated scrolling.
    enum class ScrollMode { Instant, Fast, Smooth };
    Q_ENUM(ScrollMode)

    [[nodiscard]] ScrollMode scrollMode() const;
    void setScrollMode(ScrollMode mode);

    [[nodiscard]] QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    [[nodiscard]] QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

    // Whether the left rail is hidden. Remembered across restarts, because a
    // window that forgets how the user arranged it is a window they have to fix
    // every morning.
    [[nodiscard]] bool sideBarCollapsed() const;
    void setSideBarCollapsed(bool collapsed);

    [[nodiscard]] bool restoreSessionOnStart() const;
    void setRestoreSessionOnStart(bool restore);

    // Whether Yozora may check for updates on its own (once shortly after start,
    // then periodically). Off by default: the privacy notes promise the browser
    // does not talk to the network unless asked.
    [[nodiscard]] bool backgroundUpdates() const;
    void setBackgroundUpdates(bool enabled);

    // Whether Chromium may use the GPU. Off means software rendering, which is
    // slower but avoids the GPU-compositing flicker ("black checkerboard") some
    // drivers produce. Needs a restart, so it is read before QApplication.
    [[nodiscard]] bool hardwareAcceleration() const;
    void setHardwareAcceleration(bool enabled);
    [[nodiscard]] static bool bootHardwareAcceleration();

    // --- privacy ----------------------------------------------------------
    // Drop cookies that belong to another site than the one in the address
    // bar. This is the single most effective anti-tracking cookie setting.
    [[nodiscard]] bool blockThirdPartyCookies() const;
    void setBlockThirdPartyCookies(bool block);

    // When false, cookies are kept in memory only and are gone on exit.
    [[nodiscard]] bool keepCookiesOnExit() const;
    void setKeepCookiesOnExit(bool keep);

    // Block requests that match the bundled advert and tracker filter lists.
    [[nodiscard]] bool blockAds() const;
    void setBlockAds(bool block);

    // Sites the user has turned the shield off for. Matching is by exact host,
    // so allowing "example.com" does not allow "ads.example.com".
    [[nodiscard]] QStringList adBlockAllowlist() const;
    void allowSiteForAdBlock(const QString& host);
    void disallowSiteForAdBlock(const QString& host);
    [[nodiscard]] bool isSiteAllowedForAdBlock(const QString& host) const;

    // Send the (advisory) DNT and Sec-GPC headers with every request.
    [[nodiscard]] bool sendDoNotTrack() const;
    void setSendDoNotTrack(bool send);

    // Global switch for web notifications. When off, every notification
    // permission request is denied without asking.
    [[nodiscard]] bool notificationsEnabled() const;
    void setNotificationsEnabled(bool enabled);

    enum class WebRtcPolicy {
        Default,            // Chromium's own policy
        PublicInterfaceOnly,// hide local network addresses (may break LAN calls)
        DisableNonProxiedUdp// only route WebRTC through a proxy (breaks calls without one)
    };
    Q_ENUM(WebRtcPolicy)

    [[nodiscard]] WebRtcPolicy webrtcPolicy() const;
    void setWebRtcPolicy(WebRtcPolicy policy);

    // Reads the WebRTC policy directly from the store. Safe to call before
    // QApplication exists, which is required because the value is handed to
    // Chromium through an environment variable at startup.
    [[nodiscard]] static WebRtcPolicy bootWebRtcPolicy();

    void resetToDefaults();

signals:
    void searchEngineChanged();
    void homePageChanged();
    void downloadDirectoryChanged();
    void scrollModeChanged();
    void cookiePolicyChanged();
    void adBlockingChanged();
    void doNotTrackChanged();
    void notificationsEnabledChanged();
    void webrtcPolicyChanged();
    void backgroundUpdatesChanged();
    void hardwareAccelerationChanged();

private:
    class QScopedPointer<class SettingsPrivate> d;
};

}  // namespace yozora
