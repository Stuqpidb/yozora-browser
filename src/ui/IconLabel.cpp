// SPDX-License-Identifier: MIT
#include "ui/IconLabel.h"

#include "core/Theme.h"

#include <QPainter>
#include <QPaintEvent>

namespace yozora {

IconLabel::IconLabel(icons::Shape shape, QWidget* parent)
    : QLabel(parent)
    , m_shape(shape)
{
    setFixedSize(m_size + 6, m_size + 6);
}

void IconLabel::setShape(icons::Shape shape)
{
    m_shape = shape;
    update();
}

void IconLabel::setIconSize(int size)
{
    m_size = size;
    setFixedSize(size + 6, size + 6);
    update();
}

void IconLabel::setMuted(bool muted)
{
    m_muted = muted;
    update();
}

QColor IconLabel::inkColor() const
{
    const auto c = Theme::isDark() ? Theme::darkColors() : Theme::lightColors();
    return QColor(m_muted ? c.textMuted : c.text);
}

void IconLabel::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF box((width() - m_size) / 2.0, (height() - m_size) / 2.0, m_size, m_size);
    icons::draw(painter, m_shape, box, inkColor(), 1.0);
}

}  // namespace yozora
