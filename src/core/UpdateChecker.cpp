// SPDX-License-Identifier: MIT
#include "core/UpdateChecker.h"

#include "utils/Version.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

namespace yozora {

namespace {

constexpr auto kDefaultRepo = "Stuqpidb/yozora-browser";
constexpr int kRedirects = 5;

// The installer is looked for by a loose name pattern on purpose: it has been
// called YozoraSetup-0.1.0.exe and YozoraSetup-0.2.1.exe so far, and the release
// tag is what is actually being compared.
bool looksLikeInstaller(const QString& name)
{
    return name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
           && (name.contains(QStringLiteral("setup"), Qt::CaseInsensitive)
               || name.contains(QStringLiteral("installer"), Qt::CaseInsensitive));
}

}  // namespace

ReleaseError errorForStatus(int status)
{
    switch (status) {
    case 200:
        return ReleaseError::None;
    case 404:
        // GitHub answers 404 both for a repository that exists but is private and
        // for one that does not exist at all. This app sends no credentials, so
        // the two cannot be told apart from here.
        return ReleaseError::NotFound;
    case 403:
        return ReleaseError::RateLimited;
    case 0:
        // No response at all: offline, DNS failure, TLS failure.
        return ReleaseError::Offline;
    default:
        return status >= 500 ? ReleaseError::ServerError : ReleaseError::BadPayload;
    }
}

ReleaseError parseLatestRelease(const QByteArray& json, const QVersionNumber& current,
                                ReleaseInfo* out, bool* newerThanCurrent)
{
    if (newerThanCurrent) {
        *newerThanCurrent = false;
    }

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return ReleaseError::BadPayload;
    }

    const QJsonObject release = document.object();

    // A draft or a prerelease is not something to push at somebody. The /latest
    // endpoint already excludes drafts, but excluding prereleases here as well
    // means a build marked as a prerelease cannot be offered as "the update".
    if (release.value(QStringLiteral("draft")).toBool()
        || release.value(QStringLiteral("prerelease")).toBool()) {
        return ReleaseError::BadPayload;
    }

    ReleaseInfo info;
    info.tag = release.value(QStringLiteral("tag_name")).toString();
    QString version = info.tag;
    if (version.startsWith(QLatin1Char('v'))) {
        version.remove(0, 1);
    }
    info.version = QVersionNumber::fromString(version);
    if (info.version.isNull() && !version.isEmpty()) {
        // A suffix such as "0.3.0-rc1" keeps its suffix when parsed whole; retry
        // on the numeric part before giving up on the tag.
        info.version = QVersionNumber::fromString(version.section(QLatin1Char('-'), 0, 0));
    }
    if (info.version.isNull()) {
        return ReleaseError::BadPayload;
    }

    info.notes = release.value(QStringLiteral("body")).toString().trimmed();
    info.pageUrl = QUrl(release.value(QStringLiteral("html_url")).toString());

    // Pick the installer out of the assets: the largest matching .exe wins. A
    // release can carry both a portable build and an installer, and the
    // installer is what a user on Windows wants.
    qint64 bestSize = -1;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (!looksLikeInstaller(name)) {
            continue;
        }
        const qint64 size = static_cast<qint64>(asset.value(QStringLiteral("size")).toDouble(-1));
        if (size > bestSize) {
            bestSize = size;
            info.assetName = name;
            info.assetSize = size;
            info.assetUrl = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
        }
    }

    const bool newer = info.version > current;
    if (newerThanCurrent) {
        *newerThanCurrent = newer;
    }
    if (out) {
        *out = info;
    }
    // A missing installer only matters when there is something to install; being
    // up to date is a perfectly good answer.
    if (info.assetUrl.isEmpty()) {
        return newer ? ReleaseError::NoInstaller : ReleaseError::None;
    }
    return ReleaseError::None;
}

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_current(QVersionNumber(kVersionMajor, kVersionMinor, kVersionPatch))
{
    // The GitHub Releases API, and nothing else is ever contacted.
    //
    // This only works while the repository is public. GitHub answers 404 to an
    // unauthenticated caller for a private repository, and shipping a token
    // inside the binary would hand the whole account to anyone who unzaps it, so
    // there is no third option: a release has to be reachable without
    // credentials before a browser can find its own updates.
    m_apiUrl = QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest")
                        .arg(QLatin1String(kDefaultRepo)));
    m_releasesPage = QUrl(QStringLiteral("https://github.com/%1/releases")
                              .arg(QLatin1String(kDefaultRepo)));
}

UpdateChecker::~UpdateChecker()
{
    if (m_file) {
        m_file->close();
        delete m_file;
    }
}

void UpdateChecker::setReleaseApiUrl(const QUrl& url)
{
    m_apiUrl = url;
}

QString UpdateChecker::downloadDirectory() const
{
    const QString downloads =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!downloads.isEmpty() && QDir(downloads).exists()) {
        return downloads;
    }
    return QDir::tempPath();
}

void UpdateChecker::checkNow()
{
    if (m_running || !m_apiUrl.isValid()) {
        return;
    }
    m_running = true;

    QNetworkRequest request(m_apiUrl);
    // GitHub rejects a request with no User-Agent outright.
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Yozora/%1").arg(QLatin1String(kVersionString)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(kRedirects);
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        finishCheck(status, reply->readAll());
        reply->deleteLater();
    });
}

void UpdateChecker::finishCheck(int status, const QByteArray& body)
{
    m_running = false;
    m_hasRelease = false;
    m_newerThanCurrent = false;
    m_release = ReleaseInfo{};

    ReleaseError failure = errorForStatus(status);
    if (failure != ReleaseError::None) {
        QString reason;
        switch (failure) {
        case ReleaseError::Offline:
            reason = tr("Could not reach the network. Check the connection.");
            break;
        case ReleaseError::NotFound:
            // This is the one a user can actually do something about, so it says
            // what is wrong instead of "HTTP 404".
            reason = tr("No public release found. The project's releases are not "
                        "visible without signing in, so updates cannot be fetched.");
            break;
        case ReleaseError::RateLimited:
            reason = tr("GitHub is rate-limiting this check. Try again in a few minutes.");
            break;
        case ReleaseError::ServerError:
            reason = tr("The release server is having trouble (HTTP %1).").arg(status);
            break;
        default:
            reason = tr("The release information could not be read.");
            break;
        }
        emit updateCheckFailed(reason, failure);
        emit updateCheckFinished(false);
        return;
    }

    ReleaseInfo info;
    bool newer = false;
    const ReleaseError parseError = parseLatestRelease(body, m_current, &info, &newer);
    if (parseError != ReleaseError::None && parseError != ReleaseError::NoInstaller) {
        emit updateCheckFailed(tr("The release information could not be read."), parseError);
        emit updateCheckFinished(false);
        return;
    }

    m_release = info;
    m_hasRelease = true;
    m_newerThanCurrent = newer;

    if (parseError == ReleaseError::NoInstaller) {
        // The version is newer but there is nothing to download, so the user
        // needs the release page rather than a download button that would fail.
        m_release.assetUrl.clear();
    }

    if (!newer) {
        emit updateCheckFinished(false);
        return;
    }
    emit updateAvailable(m_release);
    emit updateCheckFinished(true);
}

void UpdateChecker::downloadUpdate()
{
    // Deliberately refuses without a release of its own: the download URL comes
    // from the check, not from the caller, so this cannot be turned into a
    // fetch of an arbitrary address.
    if (m_downloading || !m_hasRelease || !m_newerThanCurrent
        || !m_release.assetUrl.isValid()) {
        return;
    }

    const QString fileName = m_release.assetName.isEmpty()
        ? QStringLiteral("YozoraSetup-%1.exe").arg(m_release.version.toString())
        : m_release.assetName;
    m_downloadPath = QDir(downloadDirectory()).absoluteFilePath(fileName);

    m_file = new QFile(m_downloadPath);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        const QString reason = tr("Could not write to %1.").arg(downloadDirectory());
        delete m_file;
        m_file = nullptr;
        emit updateDownloadFailed(reason);
        return;
    }

    QNetworkRequest request(m_release.assetUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Yozora/%1").arg(QLatin1String(kVersionString)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(kRedirects);
    // GitHub serves release downloads from objects.githubusercontent.com after a
    // redirect; without following it the app would save an empty file.

    m_download = m_network->get(request);
    m_downloading = true;
    m_expectedBytes = m_release.assetSize;
    m_receivedBytes = 0;

    connect(m_download, &QNetworkReply::downloadProgress, this,
            &UpdateChecker::handleDownloadProgress);

    connect(m_download, &QNetworkReply::finished, this, [this] {
        const int status = m_download->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QNetworkReply::NetworkError error = m_download->error();

        if (m_file) {
            m_file->close();
        }
        m_download->deleteLater();
        m_download = nullptr;

        if (error != QNetworkReply::NoError || status != 200) {
            failDownload(status == 0
                             ? tr("The download could not be started. Check the connection.")
                             : tr("The download failed (HTTP %1).").arg(status));
            return;
        }

        if (m_expectedBytes > 0) {
            const qint64 actual = QFileInfo(m_downloadPath).size();
            if (actual != m_expectedBytes) {
                // A truncated installer is worse than no installer: the size the
                // release page advertises is the only integrity signal available
                // without signing the binaries, so it is checked before the file
                // is handed over.
                failDownload(tr("The download was incomplete (%1 of %2 bytes).")
                                 .arg(actual).arg(m_expectedBytes));
                return;
            }
        }

        delete m_file;
        m_file = nullptr;
        m_downloading = false;
        emit updateDownloadFinished(m_downloadPath, true);
    });
}

void UpdateChecker::handleDownloadProgress(qint64 received, qint64 total)
{
    m_receivedBytes = received;
    if (m_file) {
        m_file->write(m_download->readAll());
    }
    // GitHub usually does not send a length for a redirected asset, so the size
    // the release page reported is the better number to show.
    emit updateDownloadProgress(received, total > 0 ? total : m_expectedBytes);
}

void UpdateChecker::failDownload(const QString& reason)
{
    if (m_file) {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
    m_downloading = false;
    if (m_download) {
        m_download->abort();
        m_download->deleteLater();
        m_download = nullptr;
    }
    // A partial file is never left behind: the next attempt would have to deal
    // with it, and a half-written installer on disk is a trap.
    QFile::remove(m_downloadPath);
    emit updateDownloadFailed(reason);
}

void UpdateChecker::cancelDownload()
{
    if (!m_downloading) {
        return;
    }
    failDownload(tr("The download was cancelled."));
}

}  // namespace yozora
