// SPDX-License-Identifier: MIT
//
// Address-bar input classification: the single most user-visible piece of
// logic in the browser. Getting "example.com" right and "how to bake bread"
// right is what makes the address bar feel right.

#include "core/SearchEngine.h"
#include "utils/UrlUtils.h"

#include <QTest>

using namespace yozora;

class TestUrlUtils : public QObject {
    Q_OBJECT

private slots:
    void classify_data();
    void classify();

    void withScheme_data();
    void withScheme();

    void normalize_data();
    void normalize();

    void toDisplayString_data();
    void toDisplayString();

    void searchQueryIsEscaped();
    void searchEngineProducesUrl();
    void unknownEngineFallsBackToDefault();
};

void TestUrlUtils::classify_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("expected");

    using K = url::InputKind;

    QTest::newRow("empty") << QString() << int(K::Empty);
    QTest::newRow("spaces") << QStringLiteral("   ") << int(K::Empty);
    QTest::newRow("plain host") << QStringLiteral("example.com") << int(K::Url);
    QTest::newRow("host with www") << QStringLiteral("www.example.com") << int(K::Url);
    QTest::newRow("host with path") << QStringLiteral("example.com/path?q=1") << int(K::Url);
    QTest::newRow("host with port") << QStringLiteral("localhost:8080") << int(K::Url);
    QTest::newRow("ipv4") << QStringLiteral("127.0.0.1:3000") << int(K::Url);
    QTest::newRow("explicit https") << QStringLiteral("https://example.com") << int(K::Url);
    QTest::newRow("file url") << QStringLiteral("file:///c:/temp/a.txt") << int(K::Url);
    QTest::newRow("single word") << QStringLiteral("weather") << int(K::Search);
    QTest::newRow("two words") << QStringLiteral("how to bake bread") << int(K::Search);
    QTest::newRow("question") << QStringLiteral("what is qt webengine?") << int(K::Search);
    QTest::newRow("cyrillic") << QStringLiteral("погода в москве") << int(K::Search);
    QTest::newRow("looks like domain") << QStringLiteral("hello.world") << int(K::Url);
    QTest::newRow("dot in sentence") << QStringLiteral("node.js vs node js")
                                     << int(K::Search);
    QTest::newRow("mailto") << QStringLiteral("mailto:a@example.com") << int(K::Url);
    QTest::newRow("ftp") << QStringLiteral("ftp://example.com/file") << int(K::Url);
}

void TestUrlUtils::classify()
{
    QFETCH(QString, input);
    QFETCH(int, expected);

    QCOMPARE(static_cast<int>(url::classify(input)), expected);
}

void TestUrlUtils::withScheme_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("host") << QStringLiteral("example.com")
                          << QStringLiteral("https://example.com");
    QTest::newRow("uppercase") << QStringLiteral("Example.COM")
                               << QStringLiteral("https://Example.COM");
    QTest::newRow("path") << QStringLiteral("example.com/a/b")
                          << QStringLiteral("https://example.com/a/b");
    QTest::newRow("query") << QStringLiteral("example.com?a=1")
                           << QStringLiteral("https://example.com?a=1");
    QTest::newRow("port") << QStringLiteral("localhost:8080")
                          << QStringLiteral("https://localhost:8080");
    QTest::newRow("already https") << QStringLiteral("https://example.com")
                                   << QStringLiteral("https://example.com");
    QTest::newRow("already http") << QStringLiteral("http://example.com")
                                  << QStringLiteral("http://example.com");
    QTest::newRow("file") << QStringLiteral("file:///tmp") << QStringLiteral("file:///tmp");
    QTest::newRow("protocol relative") << QStringLiteral("//example.com/x")
                                        << QStringLiteral("https://example.com/x");
    QTest::newRow("text") << QStringLiteral("hello world")
                          << QStringLiteral("hello world");
    QTest::newRow("single word") << QStringLiteral("weather") << QStringLiteral("weather");
    QTest::newRow("path only") << QStringLiteral("/foo/bar") << QStringLiteral("/foo/bar");
}

void TestUrlUtils::withScheme()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(url::withScheme(input), expected);
}

void TestUrlUtils::normalize_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    QTest::newRow("host") << QStringLiteral("example.com")
                          << QStringLiteral("https://example.com");
    QTest::newRow("whitespace") << QStringLiteral("  example.com  ")
                               << QStringLiteral("https://example.com");
    QTest::newRow("empty") << QString() << QString();
    QTest::newRow("text") << QStringLiteral("how to bake bread") << QString();
    QTest::newRow("scheme without host") << QStringLiteral("http://") << QString();
    QTest::newRow("about page") << QStringLiteral("about:yozora")
                                << QStringLiteral("about:yozora");
}

void TestUrlUtils::normalize()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    QCOMPARE(url::normalize(input), expected);
}

void TestUrlUtils::toDisplayString_data()
{
    QTest::addColumn<QString>("url");
    QTest::addColumn<QString>("expected");

    QTest::newRow("https") << QStringLiteral("https://example.com/") << QStringLiteral("example.com");
    QTest::newRow("www") << QStringLiteral("https://www.example.com/")
                         << QStringLiteral("example.com");
    QTest::newRow("http") << QStringLiteral("http://example.com/") << QStringLiteral("example.com");
    QTest::newRow("path kept") << QStringLiteral("https://example.com/a/b")
                               << QStringLiteral("example.com/a/b");
    QTest::newRow("about kept") << QStringLiteral("about:blank") << QStringLiteral("about:blank");
}

void TestUrlUtils::toDisplayString()
{
    QFETCH(QString, url);
    QFETCH(QString, expected);

    QCOMPARE(url::toDisplayString(url), expected);
}

void TestUrlUtils::searchQueryIsEscaped()
{
    QCOMPARE(url::toSearchQuery(QStringLiteral("a b&c=d")),
             QStringLiteral("a%20b%26c%3Dd"));
    QCOMPARE(url::toSearchQuery(QStringLiteral("  padded  ")), QStringLiteral("padded"));
    QCOMPARE(url::toSearchQuery(QString()), QString());
}

void TestUrlUtils::searchEngineProducesUrl()
{
    const auto engine = SearchEngines::byId(SearchEngines::kDuckDuckGoId);
    const QString target = engine.urlForQuery(url::toSearchQuery(QStringLiteral("qt webengine")));
    QVERIFY(target.startsWith(QStringLiteral("https://duckduckgo.com/")));
    QVERIFY(target.contains(QStringLiteral("q=qt%20webengine")));
    QCOMPARE(url::classify(target), url::InputKind::Url);
}

void TestUrlUtils::unknownEngineFallsBackToDefault()
{
    const auto engine = SearchEngines::byId(QStringLiteral("does-not-exist"));
    QCOMPARE(engine.id, SearchEngines::defaultId());
    QVERIFY(!engine.name.isEmpty());
    QVERIFY(engine.urlForQuery(QStringLiteral("x")).startsWith(QStringLiteral("https://")));
}

QTEST_MAIN(TestUrlUtils)
#include "tst_urlutils.moc"
