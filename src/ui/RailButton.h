// SPDX-License-Identifier: MIT
#pragma once

#include <QAbstractButton>

class QVariantAnimation;

namespace yozora {

// The shapes in the left rail. Drawing them ourselves keeps the rail
// independent of whichever font happens to be installed: the Unicode symbols
// this used to rely on (house, hourglass, gear) render at wildly different
// sizes in different fonts, and a style-sheet font size silently overrode the
// point size set in code, so the icons ended up tiny inside their buttons.
enum class RailIcon {
    Home,
    History,
    Bookmarks,
    Downloads,
    Private,
    Settings,
    Collapse,   // the chevron that hides the rail
};

// One icon in the left rail.
//
// The rail button has three visual states, and only one of them is "lit":
//
//   * idle    - a muted glyph, nothing behind it.
//   * hover   - a faint glass pill fades in while the pointer is over it, and
//               retracts when it leaves.
//   * current - the location the window is actually showing. This is the only
//               state that uses the accent colour, and it stays lit until the
//               user moves to a different place.
//
// It used to be checkable, so pressing a rail button latched it on and the
// highlight stayed for ever. Nothing in a browser stays "selected" after you
// click it, so the latch is gone.
class RailButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit RailButton(RailIcon icon, const QString& tooltip, QWidget* parent = nullptr);

    [[nodiscard]] RailIcon icon() const { return m_icon; }
    void setIcon(RailIcon icon);

    // Marks the button as the window's current location. Distinct from being
    // checkable: it is set by the window, not by the click.
    void setCurrent(bool current);
    [[nodiscard]] bool isCurrent() const { return m_current; }

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void syncAnimations();

    RailIcon m_icon;
    bool m_current = false;
    qreal m_hover = 0.0;   // 0..1, animated
    qreal m_press = 0.0;   // 0..1, animated
    QVariantAnimation* m_hoverAnimation = nullptr;
    QVariantAnimation* m_pressAnimation = nullptr;
};

}  // namespace yozora
