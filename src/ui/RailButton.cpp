// SPDX-License-Identifier: MIT
#include "ui/RailButton.h"

#include "core/Animation.h"
#include "core/Glass.h"
#include "core/Theme.h"

#include <QEnterEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QVariantAnimation>
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

void drawIcon(QPainter& painter, RailIcon icon, const QRectF& box, const QColor& line)
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
        case RailIcon::Collapse: {
            // A chevron pointing left, i.e. towards the edge the rail collapses
            // into. Drawn rather than taken from a font so it matches the rest.
            QPainterPath chevron;
            chevron.moveTo(box.right() - 6.0, box.top() + 4.0);
            chevron.lineTo(box.center().x() - 1.0, c.y());
            chevron.lineTo(box.right() - 6.0, box.bottom() - 4.0);
            strokePath(painter, chevron, pen);
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
    setFocusPolicy(Qt::NoFocus);
    setCursor(Qt::PointingHandCursor);
    setAttribute(Qt::WA_Hover, true);
    syncAnimations();
}

void RailButton::syncAnimations()
{
    const auto track = [this](QVariantAnimation** animation, auto apply) {
        *animation = new QVariantAnimation(this);
        Animation::configure(*animation, Animation::kQuickMs);
        connect(*animation, &QVariantAnimation::valueChanged, this, [this, apply](const QVariant& v) {
            apply(v.toReal());
            update();
        });
    };
    track(&m_hoverAnimation, [this](qreal v) { m_hover = v; });
    track(&m_pressAnimation, [this](qreal v) { m_press = v; });
}

void RailButton::setIcon(RailIcon icon)
{
    m_icon = icon;
    update();
}

void RailButton::setCurrent(bool current)
{
    if (m_current == current) {
        return;
    }
    m_current = current;
    // The current location is not checkable any more, so QAbstractButton would
    // not repaint on its own.
    update();
}

QSize RailButton::sizeHint() const
{
    return {48, 48};
}

void RailButton::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    Animation::start(m_hoverAnimation, m_hover, 1.0);
}

void RailButton::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    // A press that is released outside the button must not leave the highlight
    // stuck, so the press state follows the pointer.
    Animation::start(m_hoverAnimation, m_hover, 0.0);
    Animation::start(m_pressAnimation, m_press, 0.0);
}

void RailButton::mousePressEvent(QMouseEvent* event)
{
    QAbstractButton::mousePressEvent(event);
    if (event->button() == Qt::LeftButton) {
        Animation::start(m_pressAnimation, m_press, 1.0);
    }
}

void RailButton::mouseReleaseEvent(QMouseEvent* event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        Animation::start(m_pressAnimation, m_press, 0.0);
    }
}

void RailButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto c = Theme::colors();
    const QRectF pill = QRectF(rect()).adjusted(1, 1, -1, -1);

    // The pressed pill shrinks slightly. It is the only motion in the rail that
    // is not a fade, which is what makes a click feel like it landed on
    // something physical.
    const qreal squeeze = 1.0 - 0.07 * m_press;
    const QRectF target = pill.center().isNull()
        ? pill
        : QRectF(pill.center().x() - pill.width() * squeeze / 2.0,
                 pill.center().y() - pill.height() * squeeze / 2.0,
                 pill.width() * squeeze, pill.height() * squeeze);

    if (m_current) {
        // The current location keeps a glass pill with the accent in it. Only
        // this state is ever accent coloured.
        Glass::Recipe glass = Glass::recipe(target.width() / 2.0);
        const QColor accent(c.accent);
        glass.fill = QColor(accent.red(), accent.green(), accent.blue(), 44);
        glass.fillTop = QColor(accent.red(), accent.green(), accent.blue(), 62);
        glass.stroke = QColor(accent.red(), accent.green(), accent.blue(), 116);
        Glass::paintPanel(painter, target, glass, 1.0);
    } else if (m_hover > 0.001) {
        // Hover is a plain surface, not the accent: it says "this responds to
        // the pointer", not "you are here".
        Glass::Recipe glass = Glass::recipe(target.width() / 2.0);
        glass.fill = QColor(255, 255, 255, 14);
        glass.fillTop = QColor(255, 255, 255, 22);
        glass.stroke = QColor(255, 255, 255, 30);
        glass.grain = false;
        Glass::paintPanel(painter, target, glass, m_hover);
    }

    QColor line = m_current ? QColor(c.accent) : QColor(c.textMuted);
    if (!m_current && m_hover > 0.0) {
        // Fade the glyph towards full brightness along with the pill, so the
        // two never disagree about how "hot" the button is.
        const QColor muted(c.textMuted);
        const QColor full(c.text);
        line = QColor::fromRgbF(muted.redF() + (full.redF() - muted.redF()) * m_hover,
                                muted.greenF() + (full.greenF() - muted.greenF()) * m_hover,
                                muted.blueF() + (full.blueF() - muted.blueF()) * m_hover);
    }

    const QRectF box((width() - kBox) / 2.0, (height() - kBox) / 2.0, kBox, kBox);
    drawIcon(painter, m_icon, box, line);
}

}  // namespace yozora
