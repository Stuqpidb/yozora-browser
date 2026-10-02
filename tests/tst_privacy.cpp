// SPDX-License-Identifier: MIT
//
// Privacy and download-safety logic. These are the parts that decide what gets
// blocked and where a downloaded file may land, so they are worth protecting
// with tests: a mistake here is either a tracking leak or a path traversal.

#include "core/SearchEngine.h"
#include "privacy/DownloadSafety.h"
#include "privacy/FilterEngine.h"

#include "TestMain.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTest>

using namespace yozora;

class TestPrivacy : public QObject {
    Q_OBJECT

private slots:
    // --- filter engine ----------------------------------------------------
    void parsesDomainRules_data();
    void parsesDomainRules();
    void matchesDomainAndSubdomains();
    void ignoresUnsupportedRules();
    void parsesTypeAndPartyOptions();
    void blocksByTypeAndParty();
    void parsesSubstringRules();
    void classifiesAdsVsTrackers();
    void bundledListsLoad();

    // --- download safety --------------------------------------------------
    void sanitizesFileNames_data();
    void sanitizesFileNames();
    void recognisesDangerousFiles_data();
    void recognisesDangerousFiles();
    void picksAUniquePath();

    // --- custom search engine --------------------------------------------
    void validatesCustomSearchUrl_data();
    void validatesCustomSearchUrl();
    void buildsCustomSearchUrl();
};

void TestPrivacy::parsesDomainRules_data()
{
    QTest::addColumn<QString>("rule");
    QTest::addColumn<QString>("expected");

    QTest::newRow("adblock anchor") << QStringLiteral("||doubleclick.net^")
                                    << QStringLiteral("doubleclick.net");
    QTest::newRow("plain domain") << QStringLiteral("example.com")
                                  << QStringLiteral("example.com");
    QTest::newRow("uppercase") << QStringLiteral("Example.COM") << QStringLiteral("example.com");
    QTest::newRow("leading dot") << QStringLiteral(".tracker.example")
                                 << QStringLiteral("tracker.example");
    QTest::newRow("hosts file") << QStringLiteral("0.0.0.0 tracker.example")
                                << QStringLiteral("tracker.example");
    QTest::newRow("comment") << QStringLiteral("# just a comment") << QString();
    QTest::newRow("bang comment") << QStringLiteral("! uBlock comment") << QString();
    QTest::newRow("blank") << QStringLiteral("   ") << QString();
    QTest::newRow("with path") << QStringLiteral("||tracker.example/path^") << QString();
    QTest::newRow("wildcard domain") << QStringLiteral("*.tracker.example") << QString();
    QTest::newRow("no dot") << QStringLiteral("localhost") << QString();
}

void TestPrivacy::parsesDomainRules()
{
    QFETCH(QString, rule);
    QFETCH(QString, expected);
    FilterRule parsed;
    const bool ok = FilterEngine::parseRule(rule, QStringLiteral("tracker"), &parsed);
    QCOMPARE(ok ? parsed.value : QString(), expected);
}

void TestPrivacy::matchesDomainAndSubdomains()
{
    const FilterEngine engine = FilterEngine::fromLines(
        {QStringLiteral("||tracker.example^")},
        {QStringLiteral("||analytics.example^")});

    QVERIFY(engine.shouldBlock(QStringLiteral("https://tracker.example/a"),
                               QStringLiteral("tracker.example"), ResourceType::Script, true));
    QVERIFY(engine.shouldBlock(QStringLiteral("https://cdn.tracker.example/a"),
                               QStringLiteral("cdn.tracker.example"), ResourceType::Image, true));
    QVERIFY(engine.shouldBlock(QStringLiteral("https://ANALYTICS.EXAMPLE/a"),
                               QStringLiteral("ANALYTICS.EXAMPLE"), ResourceType::Script, true));

    QVERIFY(!engine.shouldBlock(QStringLiteral("https://example.com/a"),
                                QStringLiteral("example"), ResourceType::Script, true));
    QVERIFY(!engine.shouldBlock(QStringLiteral("https://tracker.example.evil.com/a"),
                                QStringLiteral("tracker.example.evil.com"), ResourceType::Script,
                                true));
}

void TestPrivacy::ignoresUnsupportedRules()
{
    // A path on an anchor rule is not a plain domain rule and is skipped.
    FilterRule parsed;
    QVERIFY(!FilterEngine::parseRule(QStringLiteral("||b.example/path"), QStringLiteral("ad"),
                                     &parsed));
    // A negated type option is unsupported, so the rule is skipped.
    QVERIFY(!FilterEngine::parseRule(QStringLiteral("||b.example^$~image"), QStringLiteral("ad"),
                                     &parsed));
}

void TestPrivacy::parsesTypeAndPartyOptions()
{
    FilterRule parsed;
    QVERIFY(FilterEngine::parseRule(QStringLiteral("||ads.example^$script,third-party"),
                                    QStringLiteral("ad"), &parsed));
    QVERIFY(parsed.kind == FilterRule::Kind::Domain);
    QCOMPARE(parsed.value, QStringLiteral("ads.example"));
    QVERIFY(parsed.matchThirdPartyOnly);
    QVERIFY(parsed.hasType(ResourceType::Script));
    QVERIFY(!parsed.hasType(ResourceType::Image));
}

void TestPrivacy::blocksByTypeAndParty()
{
    // Rule only matches third-party scripts.
    const FilterEngine engine = FilterEngine::fromLines(
        {QStringLiteral("||ads.example^$script,third-party")}, {});

    QVERIFY(engine.shouldBlock(QStringLiteral("https://ads.example/a.js"),
                               QStringLiteral("ads.example"), ResourceType::Script, true));
    // First party: not blocked.
    QVERIFY(!engine.shouldBlock(QStringLiteral("https://ads.example/a.js"),
                                QStringLiteral("ads.example"), ResourceType::Script, false));
    // Wrong type: not blocked.
    QVERIFY(!engine.shouldBlock(QStringLiteral("https://ads.example/a.png"),
                                QStringLiteral("ads.example"), ResourceType::Image, true));
}

void TestPrivacy::parsesSubstringRules()
{
    const FilterEngine engine =
        FilterEngine::fromLines({QStringLiteral("/pagead/")}, {});
    QVERIFY(engine.shouldBlock(QStringLiteral("https://host.example/pagead/banner.js"),
                               QStringLiteral("host.example"), ResourceType::Script, false));
    QVERIFY(!engine.shouldBlock(QStringLiteral("https://host.example/content.js"),
                                QStringLiteral("host.example"), ResourceType::Script, false));
}

void TestPrivacy::classifiesAdsVsTrackers()
{
    const FilterEngine engine = FilterEngine::fromLines({QStringLiteral("||ads.example^")},
                                                        {QStringLiteral("||track.example^")});
    QCOMPARE(engine.classify(QStringLiteral("https://ads.example/x"),
                             QStringLiteral("ads.example"), ResourceType::Script, true),
             FilterEngine::Category::Ad);
    QCOMPARE(engine.classify(QStringLiteral("https://track.example/x"),
                             QStringLiteral("track.example"), ResourceType::Script, true),
             FilterEngine::Category::Tracker);
    QCOMPARE(engine.classify(QStringLiteral("https://ok.example/x"),
                             QStringLiteral("ok.example"), ResourceType::Script, true),
             FilterEngine::Category::None);
}

void TestPrivacy::bundledListsLoad()
{
    // The bundled lists are compiled into the resource system. Loading them here
    // proves the resource paths match what FilterEngine::load() expects and that
    // the files contain real rules, so a rename or an empty file cannot silently
    // disable ad blocking in a release build.
    const FilterEngine engine = FilterEngine::load();
    QVERIFY2(engine.size() > 50, qPrintable(QStringLiteral("only %1 rules loaded")
                                                .arg(engine.size())));

    QVERIFY(engine.shouldBlock(QStringLiteral("https://doubleclick.net/ad.js"),
                               QStringLiteral("doubleclick.net"), ResourceType::Script, true));
    QVERIFY(engine.shouldBlock(QStringLiteral("https://www.google-analytics.com/collect"),
                               QStringLiteral("www.google-analytics.com"), ResourceType::Script,
                               true));
    QVERIFY(engine.shouldBlock(QStringLiteral("https://site.example/pagead/banner.png"),
                               QStringLiteral("site.example"), ResourceType::Image, true));
    // An ordinary asset on an ordinary site is not blocked.
    QVERIFY(!engine.shouldBlock(QStringLiteral("https://site.example/assets/app.js"),
                                QStringLiteral("site.example"), ResourceType::Script, true));
}

void TestPrivacy::sanitizesFileNames_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("traversal") << QStringLiteral("../../etc/passwd")
                               << QStringLiteral("passwd");
    QTest::newRow("windows traversal") << QStringLiteral("..\\..\\evil.exe")
                                       << QStringLiteral("evil.exe");
    QTest::newRow("control chars") << QStringLiteral("foo\r\nbar.txt")
                                   << QStringLiteral("foobar.txt");
    QTest::newRow("illegal chars") << QStringLiteral("a<b>c:d.txt")
                                   << QStringLiteral("a_b_c_d.txt");
    QTest::newRow("reserved con") << QStringLiteral("CON.txt") << QStringLiteral("_CON.txt");
    QTest::newRow("reserved nul") << QStringLiteral("nul") << QStringLiteral("_nul");
    QTest::newRow("trailing dot") << QStringLiteral("name.") << QStringLiteral("name");
    QTest::newRow("leading dots") << QStringLiteral("...hidden") << QStringLiteral("hidden");
    QTest::newRow("empty") << QString() << QStringLiteral("download");
    QTest::newRow("only dots") << QStringLiteral("...") << QStringLiteral("download");
    QTest::newRow("keeps spaces") << QStringLiteral("my report.pdf")
                                  << QStringLiteral("my report.pdf");
}

void TestPrivacy::sanitizesFileNames()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);
    QCOMPARE(downloads::sanitizeFileName(input), expected);
}

void TestPrivacy::recognisesDangerousFiles_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<bool>("dangerous");

    QTest::newRow("exe") << QStringLiteral("setup.exe") << true;
    QTest::newRow("msi") << QStringLiteral("app.MSI") << true;
    QTest::newRow("bat") << QStringLiteral("run.bat") << true;
    QTest::newRow("ps1") << QStringLiteral("script.ps1") << true;
    QTest::newRow("pdf") << QStringLiteral("document.pdf") << false;
    QTest::newRow("png") << QStringLiteral("photo.PNG") << false;
    QTest::newRow("tar.gz") << QStringLiteral("archive.tar.gz") << false;
    QTest::newRow("no extension") << QStringLiteral("README") << false;
}

void TestPrivacy::recognisesDangerousFiles()
{
    QFETCH(QString, name);
    QFETCH(bool, dangerous);
    QCOMPARE(downloads::isDangerousFile(name), dangerous);
}

void TestPrivacy::picksAUniquePath()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QFile existing(QDir(dir.path()).filePath(QStringLiteral("file.txt")));
    QVERIFY(existing.open(QIODevice::WriteOnly));
    existing.close();

    const QString unique = downloads::uniquePath(dir.path(), QStringLiteral("file.txt"));
    QCOMPARE(QFileInfo(unique).fileName(), QStringLiteral("file (1).txt"));
    QVERIFY(!QFileInfo::exists(unique));

    // A multi-dot name keeps only the last suffix for numbering.
    const QString multi = downloads::uniquePath(dir.path(), QStringLiteral("data.tar.gz"));
    QCOMPARE(QFileInfo(multi).fileName(), QStringLiteral("data.tar.gz"));
}

void TestPrivacy::validatesCustomSearchUrl_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<bool>("valid");

    QTest::newRow("https template") << QStringLiteral("https://example.com/search?q=%s") << true;
    QTest::newRow("http template") << QStringLiteral("http://localhost:8080/?q=%s") << true;
    QTest::newRow("no placeholder") << QStringLiteral("https://example.com/search") << false;
    QTest::newRow("bad scheme") << QStringLiteral("ftp://example.com/%s") << false;
    QTest::newRow("empty") << QString() << false;
    QTest::newRow("javascript") << QStringLiteral("javascript:%s") << false;
}

void TestPrivacy::validatesCustomSearchUrl()
{
    QFETCH(QString, url);
    QFETCH(bool, valid);
    QCOMPARE(SearchEngines::isValidCustomUrl(url), valid);
}

void TestPrivacy::buildsCustomSearchUrl()
{
    const SearchEngine engine =
        SearchEngines::custom(QStringLiteral("My engine"),
                              QStringLiteral("https://example.com/search?q=%s"));
    QCOMPARE(engine.id, QString::fromLatin1(SearchEngines::kCustomId));
    QCOMPARE(engine.name, QStringLiteral("My engine"));
    QCOMPARE(engine.urlForQuery(QStringLiteral("hello%20world")),
             QStringLiteral("https://example.com/search?q=hello%20world"));

    // An invalid custom URL leaves the query URL empty, which callers treat as
    // "fall back to the default engine".
    const SearchEngine broken =
        SearchEngines::custom(QStringLiteral("Broken"), QStringLiteral("not a url"));
    QVERIFY(broken.queryUrl.isEmpty());
}

int main(int argc, char** argv)
{
    TestPrivacy test;
    return yozora::testing::run(test, argc, argv);
}

#include "tst_privacy.moc"
