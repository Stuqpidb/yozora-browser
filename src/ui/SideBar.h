// SPDX-License-Identifier: MIT
#pragma once

#include <QWidget>

class QPropertyAnimation;
class QVariantAnimation;
class QGraphicsOpacityEffect;
class QVBoxLayout;

namespace yozora {

class RailButton;

// The Yozora left rail: a slim column of icon buttons that can be hidden to give
// the page the full width of the window.
//
// Collapsing is animated rather than instant, and the rail keeps its width in
// memory: once the user has hidden it, it stays hidden across tabs and restarts,
// because a window that forgets how you left it is annoying. The window owns
// that decision; the rail only animates and reports.
class SideBar : public QWidget {
    Q_OBJECT

public:
    explicit SideBar(QWidget* parent = nullptr);

    // Marks the home button as the window's current location.
    void setHomeActive(bool active);

    // Current collapsed state, and the animated transition to it.
    [[nodiscard]] bool isCollapsed() const { return m_collapsed; }
    void setCollapsed(bool collapsed, bool animate = true);

    // The width the rail occupies when expanded. The window uses this to decide
    // how far the collapse toggle sits from the left edge.
    [[nodiscard]] int expandedWidth() const;

signals:
    void homeRequested();
    void historyRequested();
    void bookmarksRequested();
    void downloadsRequested();
    void privateRequested();
    void settingsRequested();
    // The user asked for the rail to go away, or to come back.
    void collapsedChanged(bool collapsed);

private:
    RailButton* makeButton(RailButton* button, const QString& tooltip);
    void onCollapseToggled();

    QVBoxLayout* m_layout = nullptr;
    QList<RailButton*> m_buttons;
    RailButton* m_homeButton = nullptr;
    RailButton* m_collapseButton = nullptr;
    QPropertyAnimation* m_widthAnimation = nullptr;
    // Fades the rail in and out in step with its width, so collapsing reads as
    // one motion instead of "the icons vanish, then the panel shrinks".
    QGraphicsOpacityEffect* m_opacity = nullptr;
    QVariantAnimation* m_opacityAnimation = nullptr;
    bool m_collapsed = false;
};

}  // namespace yozora
