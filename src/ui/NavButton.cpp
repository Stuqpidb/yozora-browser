// SPDX-License-Identifier: MIT
#include "ui/NavButton.h"

#include "core/Glass.h"
#include "core/Theme.h"

#include <QPaintEvent>
#include <QPainter>

namespace yozora {

namespace {
constexpr int kSize = 38;
constexpr qreal kIcon = 21.0;
}  // namespace

NavButton::NavButton(icons::Shape shape, const QString& tooltip, QWidget* parent)
    : QAbstractButton(parent)
    , m_shape(shape)
{
    setToolTip(tooltip);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    setFixedSize(kSize, kSize);
}

void NavButton::setShape(icons::Shape shape)
{
    m_shape = shape;
    update();
}

QSize NavButton::sizeHint() const
{
    return {kSize, kSize};
}

void NavButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto c = Theme::colors();
    const QRectF pill = QRectF(rect()).adjusted(1, 1, -1, -1);
    const bool hover = underMouse() && isEnabled();
    const bool on = isChecked();

    if (hover || isDown() || on) {
        Glass::Recipe glass = Glass::recipe(pill.width() / 2.0);
        if (isDown()) {
            // A pressed control gets a slightly deeper fill so the press is
            // visible without a border.
            glass.fill = QColor(c.surfaceActive);
            glass.fillTop = QColor(c.surfaceHover);
        }
        if (on) {
            const QColor accent(c.accent);
            glass.fill = QColor(accent.red(), accent.green(), accent.blue(), 40);
            glass.stroke = QColor(accent.red(), accent.green(), accent.blue(), 110);
        }
        Glass::paintPanel(painter, pill, glass, 1.0);
    }

    QColor color = isEnabled() ? QColor(c.text) : QColor(c.textMuted);
    if (on) {
        color = QColor(c.accent);
    } else if (hover) {
        color = QColor(c.text);
    }
    if (isDown() && isEnabled()) {
        color = QColor(c.accent);
    }

    icons::draw(painter, m_shape, QRectF((width() - kIcon) / 2.0, (height() - kIcon) / 2.0, kIcon,
                                           kIcon),
                color);
}

}  // namespace yozora
