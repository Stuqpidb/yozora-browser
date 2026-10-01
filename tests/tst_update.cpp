// SPDX-License-Identifier: MIT
//
// The update check used to fail silently for anyone but the author: GitHub
// answers 404 for a private repository to an unauthenticated caller, and the
// only feedback was "HTTP 404" in a status strip that was gone before it could
// be read. Everything that decides what a release means is a pure function here,
// so it can be checked against saved payloads - including the ones that are easy
// to get wrong: a draft, a prerelease, a tag that is not a version, a release
// with no installer, and a version that is older than the running one.

#include "core/UpdateChecker.h"
#include "ui/UpdateDialog.h"

#include <QAbstractButton>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QTest>

using namespace yozora;

namespace {

const QVersionNumber kCurrent(0, 2, 1);

// The payload is built from parts rather than pasted in as one raw string: moc
// does not see a class at all once the file contains a raw string literal, so
// every test payload is spelled out here.
QByteArray payload(const QByteArray& body = QByteArray())
{
    QByteArray json;
    json += "{\n";
    json += "  \"tag_name\": \"v0.3.0\",\n";
    json += "  \"name\": \"Yozora 0.3.0\",\n";
    json += "  \"html_url\": \"https://github.com/Stuqpidb/yozora-browser/releases/tag/v0.3.0\",\n";
    json += "  \"body\": \"A faster start page.\",\n";
    json += "  \"draft\": false,\n";
    json += "  \"prerelease\": false,\n";
    json += "  \"assets\": [\n";
    json += "    { \"name\": \"YozoraSetup-0.3.0.exe\", \"size\": 123456789,\n";
    json += "      \"browser_download_url\": \"https://example.invalid/YozoraSetup-0.3.0.exe\" }\n";
    json += "  ]\n";
    json += "}";
    return json + body;
}

}  // namespace

class TestUpdate : public QObject {
    Q_OBJECT

private slots:
    void readsANewerRelease();
    void seesAnOlderReleaseAsUpToDate();
    void seesTheSameVersionAsUpToDate();
    void acceptsATagWithoutTheV();
    void refusesADraft();
    void refusesAPrerelease();
    void refusesATagThatIsNotAVersion();
    void reportsAMissingInstaller();
    void picksTheLargestInstaller();
    void ignoresNonInstallers();
    void classifiesHttpStatuses();
    void treatsStatusZeroAsOffline();
    void dialogExplainsAProblem();
    void dialogOffersADownload();
    void dialogNeverLaunchesAnything();
};

namespace {

QPushButton* buttonWithText(UpdateDialog& dialog, const QString& needle)
{
    const auto buttons = dialog.findChildren<QPushButton*>();
    for (QPushButton* button : buttons) {
        if (button->text().contains(needle, Qt::CaseInsensitive)) {
            return button;
        }
    }
    return nullptr;
}

QLabel* labelContaining(UpdateDialog& dialog, const QString& needle)
{
    const auto labels = dialog.findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->text().contains(needle, Qt::CaseInsensitive)) {
            return label;
        }
    }
    return nullptr;
}

}  // namespace

void TestUpdate::readsANewerRelease()
{
    ReleaseInfo info;
    bool newer = false;
    QCOMPARE(parseLatestRelease(payload(), kCurrent, &info, &newer),
             ReleaseError::None);
    QVERIFY(newer);
    QCOMPARE(info.version, QVersionNumber(0, 3, 0));
    QCOMPARE(info.tag, QStringLiteral("v0.3.0"));
    QCOMPARE(info.notes, QStringLiteral("A faster start page."));
    QCOMPARE(info.assetName, QStringLiteral("YozoraSetup-0.3.0.exe"));
    QCOMPARE(info.assetSize, 123456789LL);
    QCOMPARE(info.assetUrl.host(), QStringLiteral("example.invalid"));
}

void TestUpdate::seesAnOlderReleaseAsUpToDate()
{
    QByteArray json = payload().replace("v0.3.0", "v0.1.0");
    ReleaseInfo info;
    bool newer = true;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::None);
    QVERIFY2(!newer, "an older release must not be offered as an update");
}

void TestUpdate::seesTheSameVersionAsUpToDate()
{
    QByteArray json = payload().replace("v0.3.0", "v0.2.1");
    ReleaseInfo info;
    bool newer = true;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::None);
    QVERIFY2(!newer, "the running version is not an update");
}

void TestUpdate::acceptsATagWithoutTheV()
{
    QByteArray json = payload().replace("\"v0.3.0\"", "\"0.3.0\"");
    ReleaseInfo info;
    bool newer = false;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::None);
    QVERIFY(newer);
    QCOMPARE(info.version, QVersionNumber(0, 3, 0));
}

void TestUpdate::refusesADraft()
{
    QByteArray json = payload().replace("\"draft\": false", "\"draft\": true");
    ReleaseInfo info;
    bool newer = true;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::BadPayload);
    QVERIFY2(!newer, "a draft must never be offered");
}

void TestUpdate::refusesAPrerelease()
{
    QByteArray json =
        payload().replace("\"prerelease\": false", "\"prerelease\": true");
    ReleaseInfo info;
    bool newer = true;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::BadPayload);
    QVERIFY(!newer);
}

void TestUpdate::refusesATagThatIsNotAVersion()
{
    QByteArray json = payload().replace("\"v0.3.0\"", "\"nightly-build\"");
    ReleaseInfo info;
    bool newer = false;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::BadPayload);
    QVERIFY(!newer);
}

void TestUpdate::reportsAMissingInstaller()
{
    QByteArray json =
        payload().replace("\"assets\": [", "\"assets\": [] , \"unused\": [");
    ReleaseInfo info;
    bool newer = false;
    // Newer, but nothing to fetch: the caller has to send the user to the page.
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::NoInstaller);
    QVERIFY(newer);
    QVERIFY(info.assetUrl.isEmpty());
}

void TestUpdate::picksTheLargestInstaller()
{
    QByteArray json = payload().replace(
        "\"assets\": [",
        "\"assets\": [ { \"name\": \"YozoraSetup-0.3.0-portable.exe\", \"size\": 10, "
        "\"browser_download_url\": \"https://example.invalid/small.exe\" },");

    ReleaseInfo info;
    bool newer = false;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::None);
    QCOMPARE(info.assetSize, 123456789LL);
}

void TestUpdate::ignoresNonInstallers()
{
    QByteArray json = payload().replace(
        "\"assets\": [",
        "\"assets\": [ "
        "{ \"name\": \"yozora-portable.zip\", \"size\": 999999999, "
        "\"browser_download_url\": \"https://example.invalid/p.zip\" }, "
        "{ \"name\": \"checksums.txt\", \"size\": 500, "
        "\"browser_download_url\": \"https://example.invalid/c.txt\" },");

    ReleaseInfo info;
    bool newer = false;
    QCOMPARE(parseLatestRelease(json, kCurrent, &info, &newer), ReleaseError::None);
    QCOMPARE(info.assetName, QStringLiteral("YozoraSetup-0.3.0.exe"));
}

void TestUpdate::classifiesHttpStatuses()
{
    QCOMPARE(errorForStatus(200), ReleaseError::None);
    // The one that matters: this is what a private repository looks like.
    QCOMPARE(errorForStatus(404), ReleaseError::NotFound);
    QCOMPARE(errorForStatus(403), ReleaseError::RateLimited);
    QCOMPARE(errorForStatus(503), ReleaseError::ServerError);
}

void TestUpdate::treatsStatusZeroAsOffline()
{
    QCOMPARE(errorForStatus(0), ReleaseError::Offline);
}

void TestUpdate::dialogExplainsAProblem()
{
    UpdateDialog dialog;
    dialog.showProblem(QStringLiteral("No public release found."));
    QVERIFY(labelContaining(dialog, QStringLiteral("No public release")) != nullptr);
    // A failure has nothing to download, so the download button must not be
    // sitting there waiting to fail again.
    if (QPushButton* download = buttonWithText(dialog, QStringLiteral("Download"))) {
        QVERIFY2(download->isHidden(), "a failed check must not offer a download");
    } else {
        QVERIFY2(true, "no download button at all");
    }
}

void TestUpdate::dialogOffersADownload()
{
    ReleaseInfo info;
    bool newer = false;
    QVERIFY(parseLatestRelease(payload(), kCurrent, &info, &newer) == ReleaseError::None);

    UpdateDialog dialog;
    dialog.offerRelease(info);
    QVERIFY(labelContaining(dialog, QStringLiteral("0.3.0")) != nullptr);
    QVERIFY(labelContaining(dialog, QStringLiteral("A faster start page")) != nullptr);
    QPushButton* download = buttonWithText(dialog, QStringLiteral("Download"));
    QVERIFY2(download != nullptr, "an available update must offer the download");
    QVERIFY2(!download->isHidden(), "the download button must be reachable");
}

void TestUpdate::dialogNeverLaunchesAnything()
{
    // The one property that matters most: the dialog reports what the user
    // pressed, and the window decides what to do. It must not run a downloaded
    // file on its own.
    ReleaseInfo info;
    bool newer = false;
    parseLatestRelease(payload(), kCurrent, &info, &newer);

    UpdateDialog dialog;
    dialog.offerRelease(info);

    bool sawDownload = false;
    QObject::connect(&dialog, &UpdateDialog::downloadRequested,
                     [&sawDownload] { sawDownload = true; });
    if (QPushButton* download = buttonWithText(dialog, QStringLiteral("Download"))) {
        download->click();
    }
    QVERIFY2(sawDownload, "pressing Download should ask the window to fetch it");

    // Nothing was written and nothing was run: the dialog has no way to.
    QVERIFY(dialog.findChildren<QProcess*>().isEmpty());
}

QTEST_MAIN(TestUpdate)
#include "tst_update.moc"
