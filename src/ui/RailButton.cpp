// SPDX-License-Identifier: MIT
#include "ui/RailButton.h"

#include "core/Glass.h"
#include "core/Theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QtMath>

namespace yozora {

namespace {

constexpr int kBox = 24;      // logical drawing box for the glyph
constexpr qreal kStroke = 1.9;

void strokePath(QPainter& painter, const QPainterPath& path, const QPen& pen)
{
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

void drawIcon(QPainter& painter, RailIcon icon, const QRectF& box, const QColor& line, bool dark)
{
    QPen pen(line, kStroke);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    const QPointF c = box.center();

    switch (icon) {
        case RailIcon::Home: {
            QPainterPath roof;
            roof.moveTo(box.left() + 1.5, box.center().y());
            roof.lineTo(box.center().x(), box.top() + 3.0);
            roof.lineTo(box.right() - 1.5, box.center().y());
            strokePath(painter, roof, pen);
            QPainterPath body;
            body.moveTo(box.left() + 4.0, box.center().y() - 0.5);
            body.lineTo(box.left() + 4.0, box.bottom() - 1.5);
            body.lineTo(box.right() - 4.0, box.bottom() - 1.5);
            body.lineTo(box.right() - 4.0, box.center().y() - 0.5);
            strokePath(painter, body, pen);
            break;
        }
        case RailIcon::History: {
            QPainterPath arc;
            const QRectF dial(box.left() + 2.0, box.top() + 2.0, 20, 20);
            arc.arcMoveTo(dial, 60);
            arc.arcTo(dial, 60, 260);
            strokePath(painter, arc, pen);
            const QPointF tip(box.left() + 2.0, box.top() + 11.0);
            QPainterPath head;
            head.moveTo(tip.x() - 0.4, tip.y() - 3.4);
            head.lineTo(tip.x() + 3.2, tip.y() - 1.2);
            head.lineTo(tip.x() - 0.4, tip.y() + 0.6);
            strokePath(painter, head, pen);
            QPainterPath hands;
            hands.moveTo(c.x(), c.y() - 4.2);
            hands.lineTo(c.x(), c.y());
            hands.lineTo(c.x() + 3.4, c.y() + 1.6);
            strokePath(painter, hands, pen);
            break;
        }
        case RailIcon::Bookmarks: {
            QPainterPath ribbon;
            ribbon.moveTo(c.x() - 5.0, box.top() + 2.0);
            ribbon.lineTo(c.x() + 5.0, box.top() + 2.0);
            ribbon.lineTo(c.x() + 5.0, box.bottom() - 1.0);
            ribbon.lineTo(c.x(), box.bottom() - 5.0);
            ribbon.lineTo(c.x() - 5.0, box.bottom() - 1.0);
            ribbon.closeSubpath();
            strokePath(painter, ribbon, pen);
            break;
        }
        case RailIcon::Downloads: {
            QPainterPath stem;
            stem.moveTo(c.x(), box.top() + 1.5);
            stem.lineTo(c.x(), box.bottom() - 6.0);
            strokePath(painter, stem, pen);
            QPainterPath head;
            head.moveTo(c.x() - 4.0, box.bottom() - 9.5);
            head.lineTo(c.x(), box.bottom() - 5.5);
            head.lineTo(c.x() + 4.0, box.bottom() - 9.5);
            strokePath(painter, head, pen);
            QPainterPath tray;
            tray.moveTo(box.left() + 2.5, box.bottom() - 2.0);
            tray.lineTo(box.right() - 2.5, box.bottom() - 2.0);
            strokePath(painter, tray, pen);
            break;
        }
        case RailIcon::Private: {
            // The incognito hat: brim plus crown.
            QPainterPath brim;
            brim.moveTo(box.left() + 1.5, box.bottom() - 5.0);
            brim.lineTo(box.right() - 1.5, box.bottom() - 5.0);
            strokePath(painter, brim, pen);
            QPainterPath crown;
            crown.moveTo(box.left() + 4.5, box.bottom() - 5.0);
            crown.lineTo(box.left() + 6.0, box.top() + 5.0);
            crown.lineTo(box.right() - 6.0, box.top() + 5.0);
            crown.lineTo(box.right() - 4.5, box.bottom() - 5.0);
            strokePath(painter, crown, pen);
            QPainterPath crease;
            crease.moveTo(c.x() - 5.0, box.top() + 5.0);
            crease.lineTo(c.x() + 5.0, box.top() + 5.0);
            strokePath(painter, crease, pen);
            break;
        }
        case RailIcon::Sun: {
            painter.save();
            painter.setBrush(line);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(c, 4.2, 4.2);
            painter.restore();
            for (int i = 0; i < 8; ++i) {
                const qreal angle = i * M_PI / 4.0;
                QPainterPath ray;
                ray.moveTo(c.x() + std::cos(angle) * 7.4, c.y() + std::sin(angle) * 7.4);
                ray.lineTo(c.x() + std::cos(angle) * 10.4, c.y() + std::sin(angle) * 10.4);
                strokePath(painter, ray, pen);
            }
            break;
        }
        case RailIcon::Moon: {
            // A real crescent: two circular arcs closed into one outline and
            // filled. Punching a disc out of a drawn shape would leave the
            // outline of the shape visible through the hole, because the
            // rail background is translucent glass, not a flat colour.
            const qreal r = 9.0;
            const QPointF mid = box.center();
            const QPointF hole = mid + QPointF(4.6, -3.2);
            const qreal holeR = 8.2;

            QPainterPath crescent;
            // Outer edge, counter-clockwise from the top-left of the disc.
            const qreal startAngle = 132;
            crescent.arcMoveTo(QRectF(mid.x() - r, mid.y() - r, 2 * r, 2 * r), startAngle);
            crescent.arcTo(QRectF(mid.x() - r, mid.y() - r, 2 * r, 2 * r), startAngle, 236);
            // Inner edge, back the other way: the bite taken out of the moon.
            const qreal endAngle = startAngle + 236;
            crescent.arcTo(QRectF(hole.x() - holeR, hole.y() - holeR, 2 * holeR, 2 * holeR),
                           endAngle, -124);
            crescent.closeSubpath();

            painter.save();
            painter.setPen(Qt::NoPen);
            painter.setBrush(line);
            painter.drawPath(crescent);
            painter.restore();
            break;
        }
        case RailIcon::Settings: {
            QPainterPath gear;
            const int teeth = 8;
            const qreal inner = 6.0;
            const qreal outerR = 10.0;
            for (int i = 0; i < teeth * 2; ++i) {
                const qreal angle = i * M_PI / teeth;
                const qreal r = (i % 2 == 0) ? outerR : inner;
                const QPointF p(c.x() + std::cos(angle) * r, c.y() + std::sin(angle) * r);
                if (i == 0) {
                    gear.moveTo(p);
                } else {
                    gear.lineTo(p);
                }
            }
            gear.closeSubpath();
            strokePath(painter, gear, pen);
            painter.save();
            painter.setBrush(line);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(c, 2.6, 2.6);
            painter.restore();
            break;
        }
    }
}

}  // namespace

RailButton::RailButton(RailIcon icon, const QString& tooltip, QWidget* parent)
    : QAbstractButton(parent)
    , m_icon(icon)
{
    setToolTip(tooltip);
    setCheckable(true);
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    m_dark = Theme::isDark();
}

void RailButton::setIcon(RailIcon icon)
{
    m_icon = icon;
    update();
}

void RailButton::setDarkTheme(bool dark)
{
    m_dark = dark;
    update();
}

QSize RailButton::sizeHint() const
{
    return {48, 48};
}

void RailButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto c = m_dark ? Theme::darkColors() : Theme::lightColors();
    const QRectF pill = QRectF(rect()).adjusted(1, 1, -1, -1);
    const bool on = isChecked();
    const bool hover = underMouse();

    // The active item gets a real glass pill with the accent showing through,
    // so the current location is obvious at a glance.
    if (on || hover) {
        Glass::Recipe glass = Glass::recipe(m_dark, pill.width() / 2.0);
        if (on) {
            const QColor accent(c.accent);
            glass.fill = QColor(accent.red(), accent.green(), accent.blue(), m_dark ? 46 : 38);
            glass.fillTop = QColor(accent.red(), accent.green(), accent.blue(), m_dark ? 64 : 52);
            glass.stroke = QColor(accent.red(), accent.green(), accent.blue(), m_dark ? 120 : 90);
        }
        Glass::paintPanel(painter, pill, glass, 1.0);
    }

    QColor line = on ? QColor(c.accent) : QColor(c.textMuted);
    if (!on && hover) {
        line = QColor(c.text);
    }

    const QRectF box((width() - kBox) / 2.0, (height() - kBox) / 2.0, kBox, kBox);
    drawIcon(painter, m_icon, box, line, m_dark);
}

}  // namespace yozora
