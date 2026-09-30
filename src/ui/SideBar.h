// SPDX-License-Identifier: MIT
#pragma once

#include <QWidget>

class QToolButton;
class QVBoxLayout;

namespace yozora {

// The Yozora left rail: a slim column of icon buttons that never moves, the way
// a modern browser keeps navigation out of the tab strip's way.
class SideBar : public QWidget {
    Q_OBJECT

public:
    explicit SideBar(QWidget* parent = nullptr);

    // Marks the home button as the current location.
    void setHomeActive(bool active);
    // Reflects the current theme in the moon button's tooltip.
    void setDarkTheme(bool dark);

signals:
    void homeRequested();
    void historyRequested();
    void bookmarksRequested();
    void downloadsRequested();
    void privateRequested();
    void settingsRequested();
    void themeToggleRequested();

private:
    QToolButton* makeButton(const QString& glyph, const QString& tooltip, bool checkable = false);

    QVBoxLayout* m_layout = nullptr;
    QToolButton* m_homeButton = nullptr;
    QToolButton* m_themeButton = nullptr;
};

}  // namespace yozora
