// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>

class QWebEngineProfile;

namespace yozora {

// Owns the single persistent Chromium profile used by every tab.
//
// All browsing data (cookies, localStorage, cache, service workers, HTTP
// cache) lives on disk under AppPaths::profileDir(), so it survives restarts.
// This is the only place in the code base allowed to touch profile
// configuration, which keeps the "swap the engine later" path open.
class WebProfile : public QObject {
    Q_OBJECT

public:
    explicit WebProfile(QObject* parent = nullptr);
    ~WebProfile() override;

    [[nodiscard]] QWebEngineProfile* profile() const { return m_profile; }

    // Human readable path shown in the settings dialog.
    [[nodiscard]] QString storagePath() const;

    // Removes cookies, cache and local storage for all sites.
    void clearBrowsingData();

    // Memory footprint of the Chromium cache, in bytes.
    [[nodiscard]] qint64 cacheSize() const;

private:
    QWebEngineProfile* m_profile = nullptr;
};

}  // namespace yozora
