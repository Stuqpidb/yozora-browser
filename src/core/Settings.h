// SPDX-License-Identifier: MIT
#pragma once

#include "core/SearchEngine.h"

#include <QObject>
#include <QScopedPointer>
#include <QString>

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

    enum class ThemeMode { Dark, Light, System };
    Q_ENUM(ThemeMode)

    [[nodiscard]] ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);

    // When false (the default) the engine scrolls instantly, which feels
    // sharp and fast, like Chrome with smooth scrolling turned off. When true
    // the engine animates every scroll step.
    [[nodiscard]] bool smoothScrolling() const;
    void setSmoothScrolling(bool smooth);

    [[nodiscard]] QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    [[nodiscard]] QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

    [[nodiscard]] bool restoreSessionOnStart() const;
    void setRestoreSessionOnStart(bool restore);

    // --- privacy ----------------------------------------------------------
    // Drop cookies that belong to another site than the one in the address
    // bar. This is the single most effective anti-tracking cookie setting.
    [[nodiscard]] bool blockThirdPartyCookies() const;
    void setBlockThirdPartyCookies(bool block);

    // When false, cookies are kept in memory only and are gone on exit.
    [[nodiscard]] bool keepCookiesOnExit() const;
    void setKeepCookiesOnExit(bool keep);

    // Block requests to domains that appear in the bundled tracker list.
    [[nodiscard]] bool blockTrackers() const;
    void setBlockTrackers(bool block);

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
    void themeModeChanged();
    void smoothScrollingChanged();
    void cookiePolicyChanged();
    void trackerBlockingChanged();
    void doNotTrackChanged();
    void notificationsEnabledChanged();
    void webrtcPolicyChanged();

private:
    class QScopedPointer<class SettingsPrivate> d;
};

}  // namespace yozora
