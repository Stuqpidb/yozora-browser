// SPDX-License-Identifier: MIT
#include "privacy/RequestInterceptor.h"

#include "core/Settings.h"

#include <QByteArray>
#include <QUrl>
#include <QWebEngineUrlRequestInfo>

namespace yozora {

RequestInterceptor::RequestInterceptor(Settings* settings, QObject* parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_settings(settings)
{
    if (m_settings) {
        m_blockTrackers.store(m_settings->blockTrackers());
        m_sendDnt.store(m_settings->sendDoNotTrack());

        // The user can flip these while pages are open; the IO thread only ever
        // sees the atomics, so no GUI object is touched from there.
        connect(m_settings, &Settings::trackerBlockingChanged, this, [this] {
            if (m_settings) {
                setBlockTrackersEnabled(m_settings->blockTrackers());
            }
        });
        connect(m_settings, &Settings::doNotTrackChanged, this, [this] {
            if (m_settings) {
                setSendDoNotTrackEnabled(m_settings->sendDoNotTrack());
            }
        });
    }
}

void RequestInterceptor::setTrackerList(std::shared_ptr<const TrackerList> list)
{
    const std::lock_guard<std::mutex> lock(m_listMutex);
    m_list = std::move(list);
}

void RequestInterceptor::setBlockTrackersEnabled(bool enabled)
{
    m_blockTrackers.store(enabled);
}

void RequestInterceptor::setSendDoNotTrackEnabled(bool enabled)
{
    m_sendDnt.store(enabled);
}

std::shared_ptr<const TrackerList> RequestInterceptor::trackerList() const
{
    const std::lock_guard<std::mutex> lock(m_listMutex);
    return m_list;
}

void RequestInterceptor::interceptRequest(QWebEngineUrlRequestInfo& info)
{
    const QUrl url = info.requestUrl();
    const QString scheme = url.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https")
        && scheme != QLatin1String("ws") && scheme != QLatin1String("wss")) {
        // Internal pages, data:, blob:, developer tools: leave untouched.
        return;
    }

    if (m_sendDnt.load()) {
        // Advisory signals only. They are cheap, harmless, and asked for by the
        // privacy defaults; they are not a guarantee and are documented as such.
        info.setHttpHeader(QByteArrayLiteral("DNT"), QByteArrayLiteral("1"));
        info.setHttpHeader(QByteArrayLiteral("Sec-GPC"), QByteArrayLiteral("1"));
    }

    if (!m_blockTrackers.load()) {
        return;
    }

    // Never block the address the user actually asked for. A direct navigation
    // to a listed domain is a deliberate choice, not tracking.
    if (info.resourceType() == QWebEngineUrlRequestInfo::ResourceTypeMainFrame) {
        switch (info.navigationType()) {
            case QWebEngineUrlRequestInfo::NavigationTypeTyped:
            case QWebEngineUrlRequestInfo::NavigationTypeBackForward:
            case QWebEngineUrlRequestInfo::NavigationTypeReload:
                return;
            default:
                break;
        }
    }

    const std::shared_ptr<const TrackerList> list = trackerList();
    if (list && list->isBlocked(url.host())) {
        info.block(true);
        m_blocked.fetch_add(1, std::memory_order_relaxed);
    }
}

}  // namespace yozora
