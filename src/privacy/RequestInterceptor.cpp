// SPDX-License-Identifier: MIT
#include "privacy/RequestInterceptor.h"

#include "core/Settings.h"

#include <QByteArray>
#include <QUrl>
#include <QWebEngineUrlRequestInfo>

namespace yozora {

namespace {

// Maps Qt's resource type onto the filter engine's type enum.
ResourceType resourceTypeOf(QWebEngineUrlRequestInfo::ResourceType type)
{
    using T = QWebEngineUrlRequestInfo::ResourceType;
    switch (type) {
        case T::ResourceTypeScript:
            return ResourceType::Script;
        case T::ResourceTypeImage:
            return ResourceType::Image;
        case T::ResourceTypeStylesheet:
            return ResourceType::Stylesheet;
        case T::ResourceTypeFontResource:
            return ResourceType::Font;
        case T::ResourceTypeMedia:
            return ResourceType::Media;
        case T::ResourceTypeXhr:
            return ResourceType::XmlHttpRequest;
        case T::ResourceTypeSubFrame:
        case T::ResourceTypeSubResource:
            return ResourceType::Subdocument;
        case T::ResourceTypePing:
            return ResourceType::Ping;
        case T::ResourceTypeWebSocket:
            return ResourceType::Websocket;
        default:
            return ResourceType::Other;
    }
}

}  // namespace

void BlockingStats::reset()
{
    m_total = 0;
    m_perPage.clear();
}

void BlockingStats::record(const QString& pageUrl, FilterEngine::Category category)
{
    Q_UNUSED(category)
    ++m_total;
    if (!pageUrl.isEmpty()) {
        ++m_perPage[pageUrl];
    }
}

int BlockingStats::blockedForPage(const QString& pageUrl) const
{
    return m_perPage.value(pageUrl, 0);
}

RequestInterceptor::RequestInterceptor(Settings* settings, QObject* parent)
    : QWebEngineUrlRequestInterceptor(parent)
    , m_settings(settings)
{
    if (m_settings) {
        m_blockAds.store(m_settings->blockAds());
        m_sendDnt.store(m_settings->sendDoNotTrack());

        // The user can flip these while pages are open; the IO thread only ever
        // sees the atomics, so no GUI object is touched from there.
        connect(m_settings, &Settings::adBlockingChanged, this, [this] {
            if (m_settings) {
                setBlockAdsEnabled(m_settings->blockAds());
            }
        });
        connect(m_settings, &Settings::doNotTrackChanged, this, [this] {
            if (m_settings) {
                setSendDoNotTrackEnabled(m_settings->sendDoNotTrack());
            }
        });
    }
}

void RequestInterceptor::setFilterEngine(std::shared_ptr<const FilterEngine> engine)
{
    const std::lock_guard<std::mutex> lock(m_engineMutex);
    m_engine = std::move(engine);
}

void RequestInterceptor::setBlockAdsEnabled(bool enabled)
{
    m_blockAds.store(enabled);
}

void RequestInterceptor::setSendDoNotTrackEnabled(bool enabled)
{
    m_sendDnt.store(enabled);
}

std::shared_ptr<const FilterEngine> RequestInterceptor::filterEngine() const
{
    const std::lock_guard<std::mutex> lock(m_engineMutex);
    return m_engine;
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

    if (!m_blockAds.load()) {
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

    const std::shared_ptr<const FilterEngine> engine = filterEngine();
    if (!engine) {
        return;
    }

    // Third party means the request host differs from the site the user is on.
    const QString firstPartyHost = info.firstPartyUrl().host().toLower();
    const QString requestHost = url.host().toLower();
    const bool thirdParty = !firstPartyHost.isEmpty() && requestHost != firstPartyHost
                            && !requestHost.endsWith(QLatin1Char('.') + firstPartyHost);

    const ResourceType type = resourceTypeOf(info.resourceType());
    if (!engine->shouldBlock(url.toString(), requestHost, type, thirdParty)) {
        return;
    }

    info.block(true);
    m_blocked.fetch_add(1, std::memory_order_relaxed);
    // The signal is emitted from the IO thread; Qt queues it to the GUI thread
    // because the receiver lives there. Only plain values cross the boundary.
    emit requestBlocked(info.firstPartyUrl().toString(),
                        static_cast<int>(engine->classify(url.toString(), requestHost, type,
                                                          thirdParty)));
}

}  // namespace yozora
