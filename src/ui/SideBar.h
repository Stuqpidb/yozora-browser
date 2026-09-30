// SPDX-License-Identifier: MIT
#pragma once

#include <QWidget>

class QVBoxLayout;

namespace yozora {

class RailButton;

// The Yozora left rail: a slim column of icon buttons that never moves, the way
// a modern browser keeps navigation out of the tab strip's way.
class SideBar : public QWidget {
    Q_OBJECT

public:
    explicit SideBar(QWidget* parent = nullptr);

    // Marks the home button as the current location.
    void setHomeActive(bool active);
    // Repaints every icon for the current theme.
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
    RailButton* makeButton(RailButton* button, const QString& tooltip);

    QVBoxLayout* m_layout = nullptr;
    QList<RailButton*> m_buttons;
    RailButton* m_homeButton = nullptr;
    RailButton* m_themeButton = nullptr;
};

}  // namespace yozora
