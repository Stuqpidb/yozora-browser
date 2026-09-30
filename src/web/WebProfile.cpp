// SPDX-License-Identifier: MIT
#include "web/WebProfile.h"

#include "app/AppPaths.h"
#include "core/Settings.h"
#include "privacy/RequestInterceptor.h"
#include "privacy/TrackerList.h"
#include "utils/Version.h"

#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QUrl>
#include <QWebEngineCookieStore>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace yozora {

WebProfile::WebProfile(Settings* settings, QObject* parent)
    : WebProfile(settings, false, parent)
{
}

WebProfile::WebProfile(Settings* settings, bool ephemeral, QObject* parent)
    : QObject(parent)
    , m_settings(settings)
    , m_private(ephemeral)
    , m_blockThirdPartyCookies(std::make_shared<std::atomic<bool>>(true))
{
    AppPaths::ensureCreated();

    if (m_private) {
        // No storage name: Qt WebEngine then builds an off-the-record profile
        // that keeps nothing on the local machine and shares nothing with the
        // persistent profile.
        m_profile = new QWebEngineProfile(this);
    } else {
        m_profile = new QWebEngineProfile(QStringLiteral("yozora"), this);
        // Persistent storage lives next to the application data, never in a
        // temp directory, so the session survives a restart.
        m_profile->setPersistentStoragePath(AppPaths::profileDir());
        m_profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
        m_profile->setHttpCacheMaximumSize(512 * 1024 * 1024);  // 512 MB
    }

    configureProfile();
    applyPrivacySettings();

    if (m_settings) {
        connect(m_settings, &Settings::cookiePolicyChanged, this,
                &WebProfile::applyPrivacySettings);
        // Scrolling feel: only the "Smooth" mode uses the engine's own
        // animation. The default "Fast" mode animates the wheel in the view
        // itself, so the engine animator stays off to avoid double motion.
        connect(m_settings, &Settings::scrollModeChanged, this, [this] {
            if (m_profile && m_settings) {
                m_profile->settings()->setAttribute(
                    QWebEngineSettings::ScrollAnimatorEnabled,
                    m_settings->scrollMode() == Settings::ScrollMode::Smooth);
            }
        });
    }
}

WebProfile* WebProfile::createEphemeral(Settings* settings, QObject* parent)
{
    return new WebProfile(settings, true, parent);
}

WebProfile::~WebProfile() = default;

void WebProfile::configureProfile()
{
    // Web platform features. Nothing here weakens the security model: the
    // default Chromium sandbox, origin policy, TLS verification and mixed
    // content blocking all stay on.
    QWebEngineSettings* settings = m_profile->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, true);
    // The engine animator is only used in the "Smooth" scroll mode; the default
    // "Fast" mode is animated by WebView::wheelEvent() instead.
    settings->setAttribute(
        QWebEngineSettings::ScrollAnimatorEnabled,
        m_settings && m_settings->scrollMode() == Settings::ScrollMode::Smooth);

    // Yozora renders its own error page instead of Chromium's.
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);

    // Hardware acceleration is left enabled - Chromium decides at runtime
    // whether the GPU can be used and falls back to software rendering.
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

    // Third-party cookies are dropped before they ever reach storage. The
    // callback runs on the IO thread, so it only reads an atomic flag.
    const std::shared_ptr<std::atomic<bool>> block = m_blockThirdPartyCookies;
    m_profile->cookieStore()->setCookieFilter(
        [block](const QWebEngineCookieStore::FilterRequest& request) {
            if (!block->load()) {
                return true;
            }
            return !request.thirdParty;
        });

    // Network level privacy (blocklist + DNT/GPC) lives in the interceptor.
    m_interceptor = new RequestInterceptor(m_settings, this);
    m_interceptor->setTrackerList(
        std::make_shared<const TrackerList>(TrackerList::load()));
    m_profile->setUrlRequestInterceptor(m_interceptor);
}

void WebProfile::applyPrivacySettings()
{
    if (!m_profile) {
        return;
    }

    const bool blockThirdParty = m_settings ? m_settings->blockThirdPartyCookies() : true;
    m_blockThirdPartyCookies->store(blockThirdParty);

    if (!m_private) {
        // Keep logins across restarts, unless the user asked for a session that
        // forgets everything on exit.
        const bool keep = m_settings ? m_settings->keepCookiesOnExit() : true;
        m_profile->setPersistentCookiesPolicy(keep ? QWebEngineProfile::ForcePersistentCookies
                                                   : QWebEngineProfile::NoPersistentCookies);
    }

    if (m_interceptor && m_settings) {
        m_interceptor->setBlockTrackersEnabled(m_settings->blockTrackers());
        m_interceptor->setSendDoNotTrackEnabled(m_settings->sendDoNotTrack());
    }
}

QString WebProfile::storagePath() const
{
    return m_profile ? m_profile->persistentStoragePath() : QString();
}

void WebProfile::clearBrowsingData()
{
    clearCookies();
    clearCache();
    clearVisitedLinks();
}

void WebProfile::clearCookies()
{
    if (m_profile) {
        m_profile->cookieStore()->deleteAllCookies();
    }
}

void WebProfile::clearCache()
{
    if (m_profile) {
        m_profile->clearHttpCache();
    }
}

void WebProfile::clearVisitedLinks()
{
    if (m_profile) {
        m_profile->clearAllVisitedLinks();
    }
}

void WebProfile::clearPermissions()
{
    if (!m_profile) {
        return;
    }
    const QList<QWebEnginePermission> stored = m_profile->listAllPermissions();
    for (const QWebEnginePermission& permission : stored) {
        if (permission.isValid()) {
            permission.reset();
        }
    }
}

namespace {
QString permissionTypeName(QWebEnginePermission::PermissionType type)
{
    using T = QWebEnginePermission::PermissionType;
    switch (type) {
        case T::MediaAudioCapture:
            return QObject::tr("Microphone");
        case T::MediaVideoCapture:
            return QObject::tr("Camera");
        case T::MediaAudioVideoCapture:
            return QObject::tr("Camera and microphone");
        case T::DesktopVideoCapture:
            return QObject::tr("Screen sharing");
        case T::DesktopAudioVideoCapture:
            return QObject::tr("Screen and audio sharing");
        case T::MouseLock:
            return QObject::tr("Pointer lock");
        case T::Notifications:
            return QObject::tr("Notifications");
        case T::Geolocation:
            return QObject::tr("Location");
        case T::ClipboardReadWrite:
            return QObject::tr("Clipboard");
        case T::LocalFontsAccess:
            return QObject::tr("Local fonts");
        default:
            return QObject::tr("Unknown");
    }
}
}  // namespace

QList<WebProfile::StoredPermission> WebProfile::storedPermissions() const
{
    QList<StoredPermission> result;
    if (!m_profile) {
        return result;
    }
    const QList<QWebEnginePermission> stored = m_profile->listAllPermissions();
    result.reserve(stored.size());
    for (const QWebEnginePermission& permission : stored) {
        if (!permission.isValid()) {
            continue;
        }
        StoredPermission entry;
        entry.origin = permission.origin().toString();
        entry.type = permissionTypeName(permission.permissionType());
        entry.typeId = static_cast<int>(permission.permissionType());
        entry.state = permission.state() == QWebEnginePermission::State::Granted
                          ? QObject::tr("Allowed")
                          : QObject::tr("Blocked");
        result.append(entry);
    }
    return result;
}

void WebProfile::revokePermission(const QString& origin, int typeId)
{
    if (!m_profile) {
        return;
    }
    QWebEnginePermission permission = m_profile->queryPermission(
        QUrl(origin), static_cast<QWebEnginePermission::PermissionType>(typeId));
    if (permission.isValid()) {
        permission.reset();
    }
}

qint64 WebProfile::cacheSize() const
{
    return m_profile ? m_profile->httpCacheMaximumSize() : 0;
}

void WebProfile::requestSiteStoragePurge()
{
    AppPaths::ensureCreated();
    QFile marker(AppPaths::siteStoragePurgeMarker());
    if (marker.open(QIODevice::WriteOnly | QIODevice::Text)) {
        marker.write("1\n");
    }
}

void WebProfile::purgeSiteStorageIfRequested()
{
    const QString markerPath = AppPaths::siteStoragePurgeMarker();
    if (!QFile::exists(markerPath)) {
        return;
    }

    // No profile may be open yet. Removing the whole profile directory wipes
    // localStorage, IndexedDB, service workers, the cache and cookies, which is
    // exactly what an explicit "clear everything" request means.
    QDir profile(AppPaths::profileDir());
    if (profile.exists()) {
        profile.removeRecursively();
    }
    QDir().mkpath(AppPaths::profileDir());
    QFile::remove(markerPath);
}

}  // namespace yozora
