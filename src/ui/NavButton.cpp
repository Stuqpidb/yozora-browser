// SPDX-License-Identifier: MIT
#include "ui/NavButton.h"

#include "core/Animation.h"
#include "core/Glass.h"
#include "core/Theme.h"

#include <QEnterEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>

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
    syncAnimations();
}

void NavButton::syncAnimations()
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

void NavButton::setShape(icons::Shape shape)
{
    m_shape = shape;
    update();
}

void NavButton::setIconColor(const QColor& color)
{
    m_iconColor = color;
    update();
}

QSize NavButton::sizeHint() const
{
    return {kSize, kSize};
}

void NavButton::enterEvent(QEnterEvent* event)
{
    QAbstractButton::enterEvent(event);
    if (isEnabled()) {
        Animation::start(m_hoverAnimation, m_hover, 1.0);
    }
}

void NavButton::leaveEvent(QEvent* event)
{
    QAbstractButton::leaveEvent(event);
    Animation::start(m_hoverAnimation, m_hover, 0.0);
    Animation::start(m_pressAnimation, m_press, 0.0);
}

void NavButton::mousePressEvent(QMouseEvent* event)
{
    QAbstractButton::mousePressEvent(event);
    if (event->button() == Qt::LeftButton && isEnabled()) {
        Animation::start(m_pressAnimation, m_press, 1.0);
    }
}

void NavButton::mouseReleaseEvent(QMouseEvent* event)
{
    QAbstractButton::mouseReleaseEvent(event);
    if (event->button() == Qt::LeftButton) {
        Animation::start(m_pressAnimation, m_press, 0.0);
    }
}

void NavButton::changeEvent(QEvent* event)
{
    QAbstractButton::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        updateEnabledState();
    }
}

void NavButton::updateEnabledState()
{
    // A disabled toolbar button (back with no history) has to retract whatever
    // hover it had, or it keeps a highlight the user cannot act on.
    if (!isEnabled()) {
        Animation::start(m_hoverAnimation, m_hover, 0.0);
        Animation::start(m_pressAnimation, m_press, 0.0);
    }
    update();
}

void NavButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto c = Theme::colors();
    const bool on = isChecked();
    const bool live = isEnabled();

    // The pill shrinks a little while held, which is the only thing on screen
    // that says "this is being pressed right now".
    const qreal squeeze = 1.0 - 0.06 * m_press;
    const QRectF pill(QRectF(rect()).adjusted(1, 1, -1, -1));
    const QRectF target(pill.center().x() - pill.width() * squeeze / 2.0,
                        pill.center().y() - pill.height() * squeeze / 2.0,
                        pill.width() * squeeze, pill.height() * squeeze);

    if (on) {
        Glass::Recipe glass = Glass::recipe(target.width() / 2.0);
        const QColor accent(c.accent);
        glass.fill = QColor(accent.red(), accent.green(), accent.blue(), 40);
        glass.stroke = QColor(accent.red(), accent.green(), accent.blue(), 110);
        glass.grain = false;
        Glass::paintPanel(painter, target, glass, 1.0);
    } else if (m_hover > 0.001) {
        Glass::Recipe glass = Glass::recipe(target.width() / 2.0);
        // Pressed is a step darker than hovered, so a held button reads as held.
        glass.fill = m_press > 0.5 ? QColor(c.surfaceActive) : QColor(c.surfaceHover);
        glass.fillTop = m_press > 0.5 ? QColor(c.surfaceHover) : QColor(c.surfaceHover);
        glass.grain = false;
        Glass::paintPanel(painter, target, glass, m_hover);
    }

    QColor color = live ? QColor(c.text) : QColor(c.textMuted);
    if (on || m_press > 0.5) {
        color = QColor(c.accent);
    } else if (m_hover > 0.0) {
        // Same fade as the rail: the icon brightens as the pill appears.
        const QColor muted(c.textMuted);
        const QColor full(c.text);
        color = QColor::fromRgbF(muted.redF() + (full.redF() - muted.redF()) * m_hover,
                                 muted.greenF() + (full.greenF() - muted.greenF()) * m_hover,
                                 muted.blueF() + (full.blueF() - muted.blueF()) * m_hover);
    }
    // An explicit colour (the shield) wins, but still brightens on hover so it
    // does not look inert.
    if (m_iconColor.isValid()) {
        color = m_iconColor;
        if (m_hover > 0.0 && live) {
            const QColor full(c.text);
            color = QColor::fromRgbF(color.redF() + (full.redF() - color.redF()) * m_hover,
                                     color.greenF() + (full.greenF() - color.greenF()) * m_hover,
                                     color.blueF() + (full.blueF() - color.blueF()) * m_hover);
        }
    }

    const qreal icon = m_iconSize;
    icons::draw(painter, m_shape,
                QRectF((width() - icon) / 2.0, (height() - icon) / 2.0, icon, icon), color);
}

}  // namespace yozora
