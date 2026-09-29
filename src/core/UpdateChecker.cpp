// SPDX-License-Identifier: MIT
#include "core/UpdateChecker.h"

#include "utils/Version.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace yozora {

namespace {
constexpr auto kDefaultRepo = "Stuqpidb/yozora-browser";
}  // namespace

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
    , m_current(QVersionNumber(kVersionMajor, kVersionMinor, kVersionPatch))
{
    // Default endpoint: the GitHub Releases API. Nothing else is contacted.
    m_apiUrl = QUrl(QStringLiteral("https://api.github.com/repos/%1/releases/latest")
                        .arg(QLatin1String(kDefaultRepo)));
    m_releasesPage = QUrl(QStringLiteral("https://github.com/%1/releases")
                              .arg(QLatin1String(kDefaultRepo)));

    // No auto-start in the MVP: an update check is a network request, and the
    // user has not asked for one yet. It is triggered from the menu.
}

UpdateChecker::~UpdateChecker() = default;

void UpdateChecker::setReleaseApiUrl(const QUrl& url)
{
    m_apiUrl = url;
}

void UpdateChecker::checkNow()
{
    if (m_running || !m_apiUrl.isValid()) {
        return;
    }
    m_running = true;

    QNetworkRequest request(m_apiUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("Yozora/%1").arg(QLatin1String(kVersionString)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        handleResponse(status, reply->readAll());
        reply->deleteLater();
    });
}

void UpdateChecker::handleResponse(int statusCode, const QByteArray& payload)
{
    m_running = false;

    if (statusCode != 200) {
        emit updateCheckFailed(tr("Could not reach the release server (HTTP %1).").arg(statusCode));
        return;
    }

    QJsonParseError error{};
    const QJsonDocument document = QJsonDocument::fromJson(payload, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit updateCheckFailed(tr("The release information could not be read."));
        return;
    }

    const QJsonObject release = document.object();
    const QString tag = release.value(QStringLiteral("tag_name")).toString();
    const QString latest = tag.startsWith(QLatin1Char('v')) ? tag.mid(1) : tag;
    const QVersionNumber latestVersion = QVersionNumber::fromString(latest);

    if (latestVersion.isNull()) {
        emit updateCheckFailed(tr("The release information could not be read."));
        return;
    }

    if (latestVersion <= m_current) {
        emit updateCheckFinished(false);
        return;
    }

    QUrl downloadUrl = m_releasesPage;
    const QJsonArray assets = release.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue& value : assets) {
        const QJsonObject asset = value.toObject();
        const QString name = asset.value(QStringLiteral("name")).toString();
        if (name.contains(QStringLiteral("Setup"), Qt::CaseInsensitive)
            && name.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)) {
            downloadUrl = QUrl(asset.value(QStringLiteral("browser_download_url")).toString());
            break;
        }
    }

    emit updateAvailable(latestVersion.toString(), downloadUrl);
    emit updateCheckFinished(true);
}

}  // namespace yozora
