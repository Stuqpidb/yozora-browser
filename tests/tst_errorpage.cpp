// SPDX-License-Identifier: MIT
//
// The Yozora error page must never fall back to the Chromium page, and it must
// follow the shell theme. Both are easy to break without noticing, so they are
// asserted here rather than only by looking at the running browser.

#include "core/Theme.h"
#include "web/ErrorPage.h"

#include <QTest>
#include <QUrl>

using namespace yozora;

class TestErrorPage : public QObject {
    Q_OBJECT

private slots:
    void producesACompleteDocument();
    void namesTheHostThatFailed();
    void escapesTheHost();
    void darkAndLightDiffer();
    void titlesFollowTheFailure();
    void hintsExplainTheFailure();
};

void TestErrorPage::producesACompleteDocument()
{
    const QString html = ErrorPage::html(QUrl(QStringLiteral("https://example.com/x")), 0, 0,
                                         QString(), true);

    QVERIFY(html.contains(QStringLiteral("<!DOCTYPE html>")));
    QVERIFY(html.contains(QStringLiteral("</html>")));
    QVERIFY(html.contains(QStringLiteral("Try again")));
    QVERIFY(html.contains(QStringLiteral("location.reload()")));
}

void TestErrorPage::namesTheHostThatFailed()
{
    const QUrl url(QStringLiteral("https://no-such-host.invalid/a/b"));
    const QString html = ErrorPage::html(url, ErrorPage::DomainDns, ErrorPage::NameNotResolved,
                                         QString(), true);
    QVERIFY(html.contains(QStringLiteral("no-such-host.invalid")));
    QVERIFY(!html.contains(QStringLiteral("a/b")));
}

void TestErrorPage::escapesTheHost()
{
    // The error text comes from the network stack, so it is the real injection
    // vector: it must never reach the document as markup.
    const QUrl url(QStringLiteral("https://evil.example/"));
    const QString html =
        ErrorPage::html(url, 0, 0, QStringLiteral("<script>alert(1)</script>"), true);

    QVERIFY(!html.contains(QStringLiteral("<script>alert(1)</script>")));
    QVERIFY(html.contains(QStringLiteral("&lt;script&gt;alert(1)&lt;/script&gt;")));
}

void TestErrorPage::darkAndLightDiffer()
{
    const QUrl url(QStringLiteral("https://example.com/"));
    const QString dark = ErrorPage::html(url, 0, 0, QString(), true);
    const QString light = ErrorPage::html(url, 0, 0, QString(), false);

    const auto darkBg = Theme::darkColors().background;
    const auto lightBg = Theme::lightColors().background;

    QVERIFY2(dark.contains(darkBg), qPrintable("dark page misses " + darkBg));
    QVERIFY2(light.contains(lightBg), qPrintable("light page misses " + lightBg));
    QVERIFY(dark != light);
}

void TestErrorPage::titlesFollowTheFailure()
{
    QCOMPARE(ErrorPage::titleFor(ErrorPage::DomainDns, ErrorPage::NameNotResolved),
             QStringLiteral("Site not found"));
    QCOMPARE(ErrorPage::titleFor(ErrorPage::DomainCertificate, 0),
             QStringLiteral("Secure connection failed"));
    QCOMPARE(ErrorPage::titleFor(ErrorPage::DomainConnection, ErrorPage::ConnectionRefused),
             QStringLiteral("Connection refused"));
    QCOMPARE(ErrorPage::titleFor(ErrorPage::DomainConnection, ErrorPage::UnsafePort),
             QStringLiteral("This port is not allowed"));
    // An unknown net code still gets the domain's wording, never an empty title.
    QVERIFY(!ErrorPage::titleFor(ErrorPage::DomainConnection, -99999).isEmpty());
    QVERIFY(!ErrorPage::titleFor(ErrorPage::DomainNone, 0).isEmpty());
}

void TestErrorPage::hintsExplainTheFailure()
{
    QVERIFY(!ErrorPage::hintFor(ErrorPage::DomainCertificate, 0).isEmpty());
    QVERIFY(ErrorPage::hintFor(ErrorPage::DomainCertificate, 0)
                .contains(QStringLiteral("Certificate"), Qt::CaseInsensitive));
    QVERIFY(!ErrorPage::hintFor(0, 0).isEmpty());
}

QTEST_MAIN(TestErrorPage)
#include "tst_errorpage.moc"
