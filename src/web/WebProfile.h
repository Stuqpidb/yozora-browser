// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QObject>
#include <QString>

#include <atomic>
#include <memory>

class QWebEngineProfile;

namespace yozora {

class RequestInterceptor;
class Settings;

// Owns a Chromium profile and wires the privacy policy onto it.
//
// Two flavours exist:
//
//   * the persistent profile, whose cookies, storage and cache live on disk
//     under AppPaths::profileDir();
//   * an off-the-record profile for private windows, which keeps nothing on
//     disk and shares none of its state with the persistent profile.
//
// This is the only place allowed to touch profile configuration, which keeps
// the "swap the engine later" path open.
class WebProfile : public QObject {
    Q_OBJECT

public:
    // A permission as shown in the UI. Deliberately free of WebEngine types so
    // that the settings widget never has to include them.
    struct StoredPermission {
        QString origin;
        QString type;   // human readable, e.g. "Camera"
        QString state;  // "Allowed" or "Blocked"
        int typeId = 0; // opaque id, passed back to revokePermission()
    };

    explicit WebProfile(Settings* settings, QObject* parent = nullptr);
    ~WebProfile() override;

    // Creates an off-the-record profile for a private browsing window. The
    // caller owns the result.
    [[nodiscard]] static WebProfile* createEphemeral(Settings* settings, QObject* parent = nullptr);

    [[nodiscard]] QWebEngineProfile* profile() const { return m_profile; }
    [[nodiscard]] bool isPrivate() const { return m_private; }

    // Human readable path shown in the settings dialog.
    [[nodiscard]] QString storagePath() const;

    // Removes cookies, cache and local storage for all sites.
    void clearBrowsingData();

    // Granular clear operations used by the "Clear browsing data" dialog.
    void clearCookies();
    void clearCache();
    void clearVisitedLinks();

    // Forgets every stored site permission, so sites ask again.
    void clearPermissions();

    // Permissions currently stored for the profile, for the settings dialog.
    [[nodiscard]] QList<StoredPermission> storedPermissions() const;
    void revokePermission(const QString& origin, int typeId);

    // Memory footprint of the Chromium cache, in bytes.
    [[nodiscard]] qint64 cacheSize() const;

    // How many requests the interceptor has blocked this session.
    [[nodiscard]] quint64 blockedTrackerCount() const;

    // The interceptor, so the shield can connect to its blocking signal and
    // read the counters. Null for profiles without one.
    [[nodiscard]] RequestInterceptor* interceptor() const { return m_interceptor; }

    // Requests that on-disk site storage be wiped on the next start. Clearing
    // localStorage / IndexedDB / service workers while Chromium runs is not
    // supported, so the request is deferred rather than faked.
    static void requestSiteStoragePurge();

    // Performs a deferred purge. Must be called once, before any profile is
    // created.
    static void purgeSiteStorageIfRequested();

private:
    WebProfile(Settings* settings, bool ephemeral, QObject* parent);

    void configureProfile();
    void applyPrivacySettings();

    Settings* m_settings = nullptr;
    QWebEngineProfile* m_profile = nullptr;
    RequestInterceptor* m_interceptor = nullptr;
    bool m_private = false;

    // Shared with the cookie filter callback, which runs on the Chromium IO
    // thread and therefore must not touch the Settings object directly.
    std::shared_ptr<std::atomic<bool>> m_blockThirdPartyCookies;
};

}  // namespace yozora

