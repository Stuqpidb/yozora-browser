// SPDX-License-Identifier: MIT
#pragma once

#include <QIcon>
#include <QString>
#include <QTabWidget>

class QToolButton;

namespace yozora {

// The Yozora tab strip.
//
// A thin QTabWidget subclass. It owns the "+" button (placed in the tab bar's
// corner) and decides which tab should be activated after another one closes.
class TabWidget : public QTabWidget {
    Q_OBJECT

public:
    explicit TabWidget(QWidget* parent = nullptr);
    ~TabWidget() override;

    // Appends a tab and returns its index. Named appendTab() rather than
    // addTab() so it can never hide or recurse into QTabWidget::addTab().
    int appendTab(QWidget* page, const QString& title);

    // Updates title, favicon and tooltip in one go.
    void updateTab(int index, const QString& title, const QIcon& icon);

    // Index to activate after `closingIndex` is closed, or -1 when the last
    // tab is going away.
    [[nodiscard]] int preferredNextTabIndex(int closingIndex) const;

    // Title shown for a tab that has not reported one yet.
    [[nodiscard]] static QString defaultTitle() { return QStringLiteral("New Tab"); }

signals:
    void newTabRequested();
    void closeRequested(int index);

private:
    QToolButton* m_newTabButton = nullptr;
    int m_lastActiveIndex = 0;
};

}  // namespace yozora
