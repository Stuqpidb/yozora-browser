// SPDX-License-Identifier: MIT
#pragma once

#include "core/Settings.h"
#include "privacy/FilterEngine.h"

#include <QPointer>
#include <QSet>
#include <QWebEngineUrlRequestInterceptor>

#include <atomic>
#include <memory>
#include <mutex>

class QWebEngineUrlRequestInfo;

namespace yozora {

// Session counters for the shield. Total is process-wide; per-tab counts are
// keyed by the request's initiating page. All access is from the GUI thread
// through BlockingStats, so plain values are enough.
class BlockingStats {
public:
    void reset();
    void record(const QString& pageUrl, FilterEngine::Category category);

    [[nodiscard]] int blockedTotal() const { return m_total; }
    [[nodiscard]] int blockedForPage(const QString& pageUrl) const;

private:
    int m_total = 0;
    QHash<QString, int> m_perPage;
};

// Intercepts every URL request of a profile to (a) attach privacy headers and
// (b) block requests matching the ad/tracker filter lists.
//
// Important: interceptRequest() is invoked on the Chromium IO thread. It must
// never touch the GUI, Settings, or anything owned by another thread; it only
// reads immutable snapshots and atomics. The main thread updates those through
// the setter methods, so no lock is ever taken around a GUI call.
class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    explicit RequestInterceptor(Settings* settings, QObject* parent = nullptr);

    // Swaps in a freshly built engine. Passing nullptr disables matching.
    void setFilterEngine(std::shared_ptr<const FilterEngine> engine);
    void setBlockAdsEnabled(bool enabled);
    void setSendDoNotTrackEnabled(bool enabled);

    [[nodiscard]] quint64 blockedRequestCount() const { return m_blocked.load(); }
    void resetBlockedRequestCount() { m_blocked.store(0); }

    void interceptRequest(QWebEngineUrlRequestInfo& info) override;

signals:
    // Emitted on the GUI thread (queued from the IO thread) whenever a request
    // is blocked, so the shield can update. pageUrl is the top-level page the
    // request came from.
    void requestBlocked(const QString& pageUrl, int category);

private:
    [[nodiscard]] std::shared_ptr<const FilterEngine> filterEngine() const;

    QPointer<Settings> m_settings;
    mutable std::mutex m_engineMutex;
    std::shared_ptr<const FilterEngine> m_engine;
    std::atomic<bool> m_blockAds{true};
    std::atomic<bool> m_sendDnt{true};
    std::atomic<quint64> m_blocked{0};
};

}  // namespace yozora
