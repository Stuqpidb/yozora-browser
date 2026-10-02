// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QAbstractButton>
#include <QColor>

class QEnterEvent;
class QMouseEvent;
class QVariantAnimation;

namespace yozora {

// A toolbar button: a glass pill that fades in under the pointer, squashes
// slightly while held down, and a drawn icon that brightens with it.
//
// The hover and press states are animated rather than binary. A button that
// appears instantly under the cursor looks like it was already there, and one
// that appears in a fixed step looks like it blinks; a short fade reads as
// "this is responding to you".
//
// Like the rail buttons, these are not checkable. The star is the one exception
// and it is set by the window to mean "this page is bookmarked", which is a
// state of the page rather than a mode the user switched on.
class NavButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit NavButton(icons::Shape shape, const QString& tooltip, QWidget* parent = nullptr);

    void setShape(icons::Shape shape);
    void setIconSize(qreal size) { m_iconSize = size; update(); }

    // Overrides the drawn icon colour. Used by the shield to stay muted when
    // nothing is blocked even while hovered. An invalid colour restores the
    // normal palette-driven colour.
    void setIconColor(const QColor& color);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    void syncAnimations();
    // Hover and press both stop when the control is disabled: a button that
    // cannot be clicked must not invite one.
    void updateEnabledState();

    icons::Shape m_shape;
    qreal m_iconSize = 21.0;
    QColor m_iconColor;  // invalid means "use the theme colour"
    qreal m_hover = 0.0;
    qreal m_press = 0.0;
    QVariantAnimation* m_hoverAnimation = nullptr;
    QVariantAnimation* m_pressAnimation = nullptr;
};

}  // namespace yozora
