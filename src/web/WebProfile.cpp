// SPDX-License-Identifier: MIT
#include "web/WebProfile.h"

#include "app/AppPaths.h"
#include "utils/Version.h"

#include <QDir>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace yozora {

WebProfile::WebProfile(QObject* parent)
    : QObject(parent)
    , m_profile(new QWebEngineProfile(QStringLiteral("yozora"), this))
{
    AppPaths::ensureCreated();

    // Persistent storage lives next to the application data, never in a temp
    // directory, so the session survives a restart.
    m_profile->setPersistentStoragePath(AppPaths::profileDir());
    m_profile->setHttpCacheType(QWebEngineProfile::DiskHttpCache);
    m_profile->setHttpCacheMaximumSize(512 * 1024 * 1024);  // 512 MB

    // Cookies have to survive a restart, otherwise every site logs the user
    // out on every launch. No third-party cookie blocking in the MVP.
    m_profile->setPersistentCookiesPolicy(QWebEngineProfile::ForcePersistentCookies);

    // Web platform features. Nothing here weakens the security model: the
    // default Chromium sandbox, origin policy and TLS verification stay on.
    QWebEngineSettings* settings = m_profile->settings();
    settings->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
    settings->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, false);
    settings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    settings->setAttribute(QWebEngineSettings::JavascriptCanPaste, true);
    settings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, true);

    // Yozora renders its own error page instead of Chromium's.
    settings->setAttribute(QWebEngineSettings::ErrorPageEnabled, false);

    // Hardware acceleration is left enabled - Chromium decides at runtime
    // whether the GPU can be used and falls back to software rendering.
    settings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
}

WebProfile::~WebProfile() = default;

QString WebProfile::storagePath() const
{
    return m_profile ? m_profile->persistentStoragePath() : QString();
}

void WebProfile::clearBrowsingData()
{
    if (!m_profile) {
        return;
    }
    m_profile->cookieStore()->deleteAllCookies();
    m_profile->clearHttpCache();
    m_profile->clearAllVisitedLinks();
}

qint64 WebProfile::cacheSize() const
{
    return m_profile ? m_profile->httpCacheMaximumSize() : 0;
}

}  // namespace yozora
