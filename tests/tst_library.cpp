// SPDX-License-Identifier: MIT
//
// The relative timestamps in the history window are easy to get wrong in a way
// no screenshot shows: the stores keep milliseconds, so reading them as seconds
// puts every entry thousands of years in the future and the column comes out
// blank. That is asserted here rather than left to be noticed by eye.

#include "app/AppPaths.h"
#include "core/BookmarkStore.h"
#include "core/HistoryStore.h"
#include "ui/LibraryDialog.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>
#include <QTest>

using namespace yozora;

class TestLibrary : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void historyRecordsMilliseconds();
    void relativeTimeReadsMilliseconds();
    void filterFindsEntries();
    void filterTextRoundTrips();

private:
    // Where the redirected user-data root ended up. The stores resolve their
    // paths through AppPaths, which derives them from the organisation and
    // application names, so those are swapped for the duration of the test and
    // the real profile is never touched.
    QString m_root;
    QString m_originalOrganization;
};

void TestLibrary::initTestCase()
{
    m_originalOrganization = QCoreApplication::organizationName();
    QCoreApplication::setOrganizationName(QStringLiteral("YozoraTestLibrary"));
    QCoreApplication::setApplicationName(
        QStringLiteral("tst-%1").arg(QCoreApplication::applicationPid()));

    m_root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QVERIFY2(!m_root.isEmpty(), "could not redirect the user-data directory");
    QVERIFY2(!m_root.contains(QStringLiteral("Yozora Browser")),
             qPrintable(QStringLiteral("refusing to run against %1").arg(m_root)));
    QDir(m_root).removeRecursively();
}

void TestLibrary::cleanupTestCase()
{
    QDir(m_root).removeRecursively();
    QCoreApplication::setOrganizationName(m_originalOrganization);
}

void TestLibrary::historyRecordsMilliseconds()
{
    HistoryStore store;
    const QString url(QStringLiteral("https://example.com/page"));
    store.record(url, QStringLiteral("Example"));

    QCOMPARE(store.count(), 1);
    const auto entries = store.recent(10);
    QCOMPARE(entries.size(), 1);

    // Not just "non-zero": it has to be near now, in milliseconds.
    const qint64 delta = QDateTime::currentMSecsSinceEpoch() - entries.first().visitedAt;
    QVERIFY2(delta >= 0 && delta < 60'000,
             qPrintable(QStringLiteral("visitedAt is %1ms away from now").arg(delta)));
}

void TestLibrary::relativeTimeReadsMilliseconds()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // Sent through the dialog the same way a row is: milliseconds in, text out.
    QVERIFY(!LibraryDialog::relativeTime(now).isEmpty());
    QVERIFY(!LibraryDialog::relativeTime(now - 3 * 3600 * 1000).isEmpty());
    // An entry with no timestamp gets no column rather than a wrong one.
    QVERIFY(LibraryDialog::relativeTime(0).isEmpty());
}

void TestLibrary::filterFindsEntries()
{
    HistoryStore store;
    store.record(QStringLiteral("https://github.com/a/b"), QStringLiteral("A repo"));
    store.record(QStringLiteral("https://news.ycombinator.com/"), QStringLiteral("Hacker News"));

    LibraryDialog dialog(false, nullptr, &store, nullptr);
    dialog.show();
    QTest::qWait(120);

    // Two entries before filtering, and typing narrows the list. The search runs
    // on a 120ms timer, so the wait has to outlast it.
    QCOMPARE(dialog.visibleRowCount(), 2);

    dialog.setFilterText(QStringLiteral("github"));
    QTest::qWait(300);
    QCOMPARE(dialog.visibleRowCount(), 1);

    dialog.setFilterText(QStringLiteral("nothing matches this"));
    QTest::qWait(300);
    QCOMPARE(dialog.visibleRowCount(), 0);
}

void TestLibrary::filterTextRoundTrips()
{
    HistoryStore store;
    LibraryDialog dialog(false, nullptr, &store, nullptr);
    dialog.show();
    QTest::qWait(120);

    dialog.setFilterText(QStringLiteral("git"));
    QCOMPARE(dialog.filterText(), QStringLiteral("git"));
    dialog.setFilterText(QString());
    QCOMPARE(dialog.filterText(), QString());
}

QTEST_MAIN(TestLibrary)
#include "tst_library.moc"
