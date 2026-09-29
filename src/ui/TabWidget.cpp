// SPDX-License-Identifier: MIT
#include "ui/TabWidget.h"

#include <QTabBar>
#include <QToolButton>

namespace yozora {

TabWidget::TabWidget(QWidget* parent)
    : QTabWidget(parent)
{
    setDocumentMode(true);
    setMovable(true);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    setTabsClosable(true);
    setTabPosition(QTabWidget::North);

    tabBar()->setDrawBase(false);
    tabBar()->setExpanding(false);
    tabBar()->setMovable(true);

    connect(this, &QTabWidget::tabCloseRequested, this, &TabWidget::closeRequested);
    connect(this, &QTabWidget::currentChanged, this, [this](int index) {
        if (index >= 0) {
            m_lastActiveIndex = index;
        }
    });

    m_newTabButton = new QToolButton(this);
    m_newTabButton->setObjectName(QStringLiteral("tabStripNewTabButton"));
    m_newTabButton->setText(QStringLiteral("+"));
    m_newTabButton->setToolTip(tr("New tab (Ctrl+T)"));
    m_newTabButton->setAutoRaise(true);
    m_newTabButton->setFocusPolicy(Qt::NoFocus);
    m_newTabButton->setCursor(Qt::PointingHandCursor);
    m_newTabButton->setFixedSize(30, 26);
    connect(m_newTabButton, &QToolButton::clicked, this, &TabWidget::newTabRequested);
    setCornerWidget(m_newTabButton, Qt::TopRightCorner);
}

TabWidget::~TabWidget() = default;

int TabWidget::appendTab(QWidget* page, const QString& title)
{
    // QTabWidget::addTab() is called explicitly: this class deliberately does
    // not overload it, so an unqualified call would recurse forever.
    const int index = QTabWidget::addTab(page, title);
    setCurrentIndex(index);
    return index;
}

void TabWidget::updateTab(int index, const QString& title, const QIcon& icon)
{
    if (index < 0 || index >= count()) {
        return;
    }
    setTabText(index, title);
    setTabIcon(index, icon);
    setTabToolTip(index, title);
}

int TabWidget::preferredNextTabIndex(int closingIndex) const
{
    if (count() <= 1) {
        return -1;
    }
    if (closingIndex != m_lastActiveIndex) {
        return m_lastActiveIndex;
    }
    const int right = closingIndex + 1;
    return right < count() ? right : closingIndex - 1;
}

}  // namespace yozora
