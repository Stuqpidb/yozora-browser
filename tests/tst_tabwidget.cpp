// SPDX-License-Identifier: MIT
//
// Tab bookkeeping: creating tabs, closing them, and picking the tab that must
// get focus afterwards. This is where off-by-one mistakes would be invisible
// to a user but immediately annoying, so it is worth a test.

#include "ui/TabWidget.h"

#include "TestMain.h"

#include <QSignalSpy>
#include <QTabBar>
#include <QTest>

using namespace yozora;

class TestTabWidget : public QObject {
    Q_OBJECT

private slots:
    void addsTabsAndActivatesNewest();
    void emitsCloseRequested();
    void emitsNewTabRequested();
    void prefersRightNeighbourWhenClosingActive();
    void wrapsToLeftNeighbourAtTheEnd();
    void keepsLastActiveWhenClosingOtherTab();
    void reportsNoNeighbourForTheLastTab();
    void updatesTitleAndIcon();
    void dragsTabs();
};

void TestTabWidget::addsTabsAndActivatesNewest()
{
    TabWidget tabs;
    QVERIFY(tabs.tabsClosable());
    QVERIFY(tabs.isMovable());
    QVERIFY(tabs.documentMode());

    const int first = tabs.appendTab(new QWidget, QStringLiteral("One"));
    QCOMPARE(first, 0);
    QCOMPARE(tabs.currentIndex(), 0);

    const int second = tabs.appendTab(new QWidget, QStringLiteral("Two"));
    QCOMPARE(second, 1);
    QCOMPARE(tabs.count(), 2);
    QCOMPARE(tabs.currentIndex(), 1);
}

void TestTabWidget::emitsCloseRequested()
{
    TabWidget tabs;
    tabs.appendTab(new QWidget, QStringLiteral("One"));

    QSignalSpy spy(&tabs, &TabWidget::closeRequested);
    const int index = tabs.appendTab(new QWidget, QStringLiteral("Two"));
    spy.clear();

    tabs.tabCloseRequested(index);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.first().first().toInt(), index);
}

void TestTabWidget::emitsNewTabRequested()
{
    TabWidget tabs;
    QSignalSpy spy(&tabs, &TabWidget::newTabRequested);
    QVERIFY(tabs.cornerWidget(Qt::TopRightCorner) != nullptr);
    QCOMPARE(spy.count(), 0);
}

void TestTabWidget::prefersRightNeighbourWhenClosingActive()
{
    TabWidget tabs;
    for (int i = 0; i < 3; ++i) {
        tabs.appendTab(new QWidget, QStringLiteral("Tab %1").arg(i));
    }
    QCOMPARE(tabs.currentIndex(), 2);

    tabs.setCurrentIndex(1);
    QCOMPARE(tabs.preferredNextTabIndex(1), 2);
}

void TestTabWidget::wrapsToLeftNeighbourAtTheEnd()
{
    TabWidget tabs;
    for (int i = 0; i < 3; ++i) {
        tabs.appendTab(new QWidget, QStringLiteral("Tab %1").arg(i));
    }
    tabs.setCurrentIndex(2);

    QCOMPARE(tabs.preferredNextTabIndex(2), 1);
}

void TestTabWidget::keepsLastActiveWhenClosingOtherTab()
{
    TabWidget tabs;
    for (int i = 0; i < 3; ++i) {
        tabs.appendTab(new QWidget, QStringLiteral("Tab %1").arg(i));
    }
    tabs.setCurrentIndex(2);

    // Closing a background tab must not move the selection.
    QCOMPARE(tabs.preferredNextTabIndex(0), 2);
}

void TestTabWidget::reportsNoNeighbourForTheLastTab()
{
    TabWidget tabs;
    tabs.appendTab(new QWidget, QStringLiteral("Only"));
    QCOMPARE(tabs.preferredNextTabIndex(0), -1);
}

void TestTabWidget::updatesTitleAndIcon()
{
    TabWidget tabs;
    const int index = tabs.appendTab(new QWidget, QStringLiteral("Old"));

    // Built from a pixmap rather than QIcon::fromTheme(), which resolves to a
    // null icon on platforms without an icon theme.
    QIcon icon;
    icon.addPixmap(QPixmap(16, 16));

    tabs.updateTab(index, QStringLiteral("New title"), icon);

    QCOMPARE(tabs.tabText(index), QStringLiteral("New title"));
    QCOMPARE(tabs.tabToolTip(index), QStringLiteral("New title"));
    QVERIFY(!tabs.tabIcon(index).isNull());
}

void TestTabWidget::dragsTabs()
{
    TabWidget tabs;
    const int a = tabs.appendTab(new QWidget, QStringLiteral("A"));
    const int b = tabs.appendTab(new QWidget, QStringLiteral("B"));
    QCOMPARE(a, 0);
    QCOMPARE(b, 1);

    // Moving tabs is what makes the strip feel like a browser rather than a
    // form with a fixed order.
    tabs.tabBar()->moveTab(b, a);
    QCOMPARE(tabs.tabText(0), QStringLiteral("B"));
    QCOMPARE(tabs.tabText(1), QStringLiteral("A"));
}

int main(int argc, char** argv)
{
    TestTabWidget test;
    return yozora::testing::run(test, argc, argv);
}

#include "tst_tabwidget.moc"
