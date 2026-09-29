// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QUrl>
#include <QVersionNumber>

class QNetworkAccessManager;

namespace yozora {

// Architecture for the planned auto-update flow. The MVP only *detects* a
// newer release on GitHub and reports it; it never downloads or installs
// anything yet.
//
//   Yozora -> checks its version -> GitHub Releases -> finds a newer tag
//           -> (future) downloads an update package -> (future) restarts
//
// GitHub Releases is the single source of official releases. Yozora does not
// run its own update server.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    // Where releases are published.
    void setReleaseApiUrl(const QUrl& url);
    [[nodiscard]] QUrl releaseApiUrl() const { return m_apiUrl; }

    [[nodiscard]] QVersionNumber currentVersion() const { return m_current; }

    // Asks GitHub for the latest release. Emits updateAvailable() or
    // updateCheckFailed() when the answer arrives. Does nothing when offline
    // or when a check is already running.
    void checkNow();

    // The page a user should open to download an update manually.
    [[nodiscard]] QUrl releasesPageUrl() const { return m_releasesPage; }

signals:
    void updateAvailable(const QString& version, const QUrl& downloadUrl);
    void updateCheckFailed(const QString& reason);
    void updateCheckFinished(bool anUpdateExists);

private:
    void handleResponse(int statusCode, const QByteArray& payload);

    QNetworkAccessManager* m_network = nullptr;
    QUrl m_apiUrl;
    QUrl m_releasesPage;
    QVersionNumber m_current;
    bool m_running = false;
};

}  // namespace yozora
