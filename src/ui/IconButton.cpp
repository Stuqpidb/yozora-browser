// SPDX-License-Identifier: MIT
#include "ui/IconButton.h"

#include "core/Theme.h"

#include <QPainter>
#include <QPaintEvent>

namespace yozora {

IconButton::IconButton(icons::Shape shape, QWidget* parent)
    : QAbstractButton(parent)
    , m_shape(shape)
{
    setAttribute(Qt::WA_Hover, true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setFixedSize(m_size + 12, m_size + 12);
}

void IconButton::setShape(icons::Shape shape)
{
    m_shape = shape;
    update();
}

void IconButton::setIconSize(int size)
{
    m_size = size;
    setFixedSize(size + 12, size + 12);
    update();
}

void IconButton::setMuted(bool muted)
{
    m_muted = muted;
    update();
}

QColor IconButton::inkColor() const
{
    const auto c = Theme::isDark() ? Theme::darkColors() : Theme::lightColors();
    if (!m_muted) {
        return QColor(c.text);
    }
    return underMouse() ? QColor(c.text) : QColor(c.textMuted);
}

QSize IconButton::sizeHint() const
{
    return QSize(m_size + 12, m_size + 12);
}

void IconButton::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (underMouse()) {
        const auto c = Theme::isDark() ? Theme::darkColors() : Theme::lightColors();
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(c.surfaceHover));
        painter.drawRoundedRect(rect(), 8.0, 8.0);
    }

    const QRectF box((width() - m_size) / 2.0, (height() - m_size) / 2.0, m_size, m_size);
    icons::draw(painter, m_shape, box, inkColor(), 1.0);
}

}  // namespace yozora
