// SPDX-License-Identifier: MIT
#pragma once

#include "core/Settings.h"
#include "privacy/TrackerList.h"

#include <QPointer>
#include <QWebEngineUrlRequestInterceptor>

#include <atomic>
#include <memory>
#include <mutex>

class QWebEngineUrlRequestInfo;

namespace yozora {

// Intercepts every URL request of a profile to (a) attach privacy headers and
// (b) block requests to known tracker domains.
//
// Important: interceptRequest() is invoked on the Chromium IO thread. It must
// never touch the GUI, Settings, or anything owned by another thread; it only
// reads immutable snapshots and atomics. The main thread updates those through
// the setter methods, so no lock is ever taken around a GUI call.
class RequestInterceptor : public QWebEngineUrlRequestInterceptor {
    Q_OBJECT

public:
    explicit RequestInterceptor(Settings* settings, QObject* parent = nullptr);

    // Swaps in a freshly built list. Passing nullptr disables matching.
    void setTrackerList(std::shared_ptr<const TrackerList> list);
    void setBlockTrackersEnabled(bool enabled);
    void setSendDoNotTrackEnabled(bool enabled);

    [[nodiscard]] quint64 blockedRequestCount() const { return m_blocked.load(); }
    void resetBlockedRequestCount() { m_blocked.store(0); }

    void interceptRequest(QWebEngineUrlRequestInfo& info) override;

private:
    [[nodiscard]] std::shared_ptr<const TrackerList> trackerList() const;

    QPointer<Settings> m_settings;
    mutable std::mutex m_listMutex;
    std::shared_ptr<const TrackerList> m_list;
    std::atomic<bool> m_blockTrackers{true};
    std::atomic<bool> m_sendDnt{true};
    std::atomic<quint64> m_blocked{0};
};

}  // namespace yozora
