// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVersionNumber>

class QFile;
class QNetworkAccessManager;
class QNetworkReply;

namespace yozora {

// What a release tells us. Kept as a plain struct so it can be built and
// compared in a test without a network.
struct ReleaseInfo {
    QVersionNumber version;
    QString tag;
    QString notes;
    QString assetName;      // the installer picked out of the release
    qint64 assetSize = -1;  // bytes, or -1 when the server did not say
    QUrl assetUrl;
    QUrl pageUrl;
};

// Why a check could not produce an answer. The distinction matters to the user:
// "you are offline" and "the project has no public releases" call for completely
// different actions, and a single generic "update failed" hides both.
enum class ReleaseError {
    None,
    Offline,
    NotFound,      // 404 - the repository or release is not publicly visible
    RateLimited,   // 403 with the rate limit headers exhausted
    ServerError,   // 5xx
    BadPayload,    // 200 with something that is not a release
    NoInstaller,   // the release exists but ships no Windows installer
};

// Parses a GitHub "latest release" payload into a ReleaseInfo.
//
// Split out of the network code on purpose: the parsing is where the bugs were
// (an empty payload, a draft release, an asset list with no installer), and it
// is the part that can be checked without asking anyone's server for anything.
ReleaseError parseLatestRelease(const QByteArray& json, const QVersionNumber& current,
                                ReleaseInfo* out, bool* newerThanCurrent);

// Maps an HTTP status onto an error. `offline` short-circuits: Qt reports a
// connection failure as status 0, which would otherwise look like success with
// an empty body.
ReleaseError errorForStatus(int status);

// Checks GitHub Releases for a newer version and, on request, downloads the
// installer.
//
// What this does *not* do, deliberately:
//
//   * It does not run on startup. An update check is a network request to a third
//     party, and the privacy notes promise that Yozora does not do that without
//     the user asking. It stays behind an explicit click.
//   * It does not execute anything. The download is verified against the size
//     GitHub reported and then handed to the user; running an installer is a
//     decision a person makes, and a browser that quietly launched a downloaded
//     executable would be a very unpleasant thing to ship.
//   * It carries no credentials. The repository has to be public for this to
//     work at all - see the note in UpdateChecker::UpdateChecker.
class UpdateChecker : public QObject {
    Q_OBJECT

public:
    explicit UpdateChecker(QObject* parent = nullptr);
    ~UpdateChecker() override;

    // Where releases are published.
    void setReleaseApiUrl(const QUrl& url);
    [[nodiscard]] QUrl releaseApiUrl() const { return m_apiUrl; }
    [[nodiscard]] QUrl releasesPageUrl() const { return m_releasesPage; }

    [[nodiscard]] QVersionNumber currentVersion() const { return m_current; }

    // Asks GitHub for the latest release. Does nothing when a check is already
    // running.
    void checkNow();

    // Starts downloading the installer found by the last successful check.
    // Refuses when there is nothing newer, so it cannot be used to fetch an
    // arbitrary URL.
    void downloadUpdate();

    // Abandons a download in progress and removes the partial file.
    void cancelDownload();

    [[nodiscard]] bool isDownloading() const { return m_downloading; }
    [[nodiscard]] bool isChecking() const { return m_running; }

    // Where a downloaded installer is put. Defaults to the user's Downloads
    // folder, falling back to the temporary folder if that is not writable.
    [[nodiscard]] QString downloadDirectory() const;

signals:
    void updateAvailable(const ReleaseInfo& info);
    void updateCheckFailed(const QString& reason, ReleaseError error);
    void updateCheckFinished(bool anUpdateExists);

    void updateDownloadStarted(const QString& fileName, qint64 totalBytes);
    void updateDownloadProgress(qint64 received, qint64 total);
    void updateDownloadFinished(const QString& path, bool sizeMatched);
    void updateDownloadFailed(const QString& reason);

private:
    void finishCheck(int status, const QByteArray& body);
    void handleDownloadProgress(qint64 received, qint64 total);
    void failDownload(const QString& reason);

    QNetworkAccessManager* m_network = nullptr;
    QUrl m_apiUrl;
    QUrl m_releasesPage;
    QVersionNumber m_current;
    ReleaseInfo m_release;      // the release found by the last check
    bool m_hasRelease = false;
    bool m_newerThanCurrent = false;
    bool m_running = false;

    QNetworkReply* m_download = nullptr;
    QFile* m_file = nullptr;
    QString m_downloadPath;
    bool m_downloading = false;
    qint64 m_expectedBytes = -1;
    qint64 m_receivedBytes = 0;
};

}  // namespace yozora
