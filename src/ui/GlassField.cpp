// SPDX-License-Identifier: MIT
#include "ui/GlassField.h"

#include "core/Animation.h"
#include "core/Glass.h"
#include "core/Theme.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>
#include <QWidget>

namespace yozora {

GlassField::GlassField(QWidget* parent)
    : QFrame(parent)
{
    setAttribute(Qt::WA_Hover, true);
    setFrameShape(QFrame::NoFrame);

    m_editor = new QLineEdit(this);
    m_editor->setObjectName(QStringLiteral("glassFieldEditor"));
    m_editor->setFrame(false);
    m_editor->installEventFilter(this);
    // The application style sheet paints every QLineEdit with a field
    // background, which would sit on top of the glass. A widget level sheet
    // wins over the application one, so the editor stays invisible.
    m_editor->setStyleSheet(QStringLiteral("background: transparent; border: none; padding: 0;"));

    connect(m_editor, &QLineEdit::returnPressed, this, &GlassField::returnPressed);
    connect(m_editor, &QLineEdit::textChanged, this, &GlassField::textChanged);
    connect(m_editor, &QLineEdit::editingFinished, this, &GlassField::editingFinished);
    // The focus ring is animated from the container, so it needs to hear about
    // focus changes: the editor is the widget that actually gets the focus.
    // QLineEdit has no focus signals, so the container's event filter is used -
    // it already sees every event the editor gets.
    layoutEditor();
    syncAnimations();
}

QString GlassField::text() const
{
    return m_editor->text();
}

void GlassField::setText(const QString& text)
{
    m_editor->setText(text);
}

void GlassField::clear()
{
    m_editor->clear();
}

void GlassField::setPlaceholderText(const QString& text)
{
    m_editor->setPlaceholderText(text);
}

void GlassField::setFocus()
{
    m_editor->setFocus();
}

void GlassField::setFocus(Qt::FocusReason reason)
{
    m_editor->setFocus(reason);
}

void GlassField::setFocusPolicy(Qt::FocusPolicy policy)
{
    QFrame::setFocusPolicy(policy);
    m_editor->setFocusPolicy(policy);
}

void GlassField::setCursorPosition(int position)
{
    m_editor->setCursorPosition(position);
}

void GlassField::selectAll()
{
    m_editor->selectAll();
}

bool GlassField::hasFocus() const
{
    return m_editor->hasFocus();
}

void GlassField::setTrailing(Trailing trailing)
{
    if (m_trailing == trailing) {
        return;
    }
    m_trailing = trailing;
    layoutEditor();
    update();
}

void GlassField::setRadius(qreal radius)
{
    m_radius = radius;
    update();
}

void GlassField::setShadowMargin(qreal margin)
{
    if (qFuzzyCompare(m_margin, margin)) {
        return;
    }
    m_margin = margin;
    layoutEditor();
    update();
}

void GlassField::setHeroMode(bool hero)
{
    if (m_hero == hero) {
        return;
    }
    m_hero = hero;
    layoutEditor();
    update();
}

void GlassField::syncAnimations()
{
    m_focusAnimation = new QVariantAnimation(this);
    Animation::configure(m_focusAnimation, Animation::kQuickMs);
    connect(m_focusAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_focus = v.toReal();
        update();
    });
}

qreal GlassField::iconSize() const
{
    // The icons scale with the field, so a 40px toolbar field and a 56px
    // start-page field get proportionally sized glyphs.
    return qBound(18.0, surfaceRect().height() * (m_hero ? 0.44 : 0.50), 28.0);
}

QRectF GlassField::trailingBox() const
{
    const qreal icon = iconSize();
    const qreal pad = m_hero ? 20.0 : 14.0;
    const QRectF surface = surfaceRect();
    return QRectF(surface.right() - pad - icon, surface.top() + (surface.height() - icon) / 2.0,
                  icon, icon);
}

QRectF GlassField::surfaceRect() const
{
    return QRectF(m_margin, m_margin, width() - 2 * m_margin, height() - 2 * m_margin);
}

void GlassField::layoutEditor()
{
    const qreal icon = iconSize();
    const qreal pad = m_hero ? 20.0 : 14.0;
    const qreal gap = m_hero ? 13.0 : 9.0;

    const int left = qRound(pad + icon + gap);
    const int right = m_trailing == Trailing::None
        ? qRound(pad)
        : qRound(pad + icon + gap);

    const QRectF surface = surfaceRect();
    // The editor is inset vertically as well. A child widget paints after the
    // parent, so if it reaches the top and bottom of the pill it draws its own
    // one-pixel border right on top of the surface's - which is why the focus
    // ring used to appear on the left and right ends but not across the middle
    // of the top and bottom edges.
    const int inset = 3;
    const QRect area = surface.toRect().adjusted(left, inset, -right, -inset);
    if (area.width() > 0) {
        m_editor->setGeometry(area);
    }
}

void GlassField::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    layoutEditor();
}

bool GlassField::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_editor) {
        if (event->type() == QEvent::FocusIn) {
            emit editorFocused();
            // The ring is animated, so the repaint has to be requested here:
            // the editor took the focus, not the container.
            update();
        } else if (event->type() == QEvent::FocusOut) {
            update();
        } else if (event->type() == QEvent::KeyPress) {
            auto* key = static_cast<QKeyEvent*>(event);
            if (key->key() == Qt::Key_Escape && !m_editor->text().isEmpty()) {
                // Escape clears the field first, which is what a user expects
                // before it means "give my focus back to the page".
                m_editor->clear();
                return true;
            }
        }
    }
    return QFrame::eventFilter(watched, event);
}

void GlassField::mousePressEvent(QMouseEvent* event)
{
    if (m_trailing != Trailing::None && event->button() == Qt::LeftButton
        && trailingBox().contains(event->position())) {
        emit trailingClicked();
        event->accept();
        return;
    }
    // The editor does not cover the leading icon, so a click there still has
    // to land in the text.
    m_editor->setFocus();
    QFrame::mousePressEvent(event);
}
void GlassField::drawIcon(QPainter& painter, icons::Shape shape, const QRectF& box,
                          const QColor& color) const
{
    icons::draw(painter, shape, box, color, m_hero ? 2.1 : 2.2);
}

void GlassField::paintEvent(QPaintEvent* event)
{
    const auto c = Theme::colors();
    const Glass::Recipe glass = Glass::recipe(m_radius);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The pill is inset by the shadow margin so the blur has somewhere to go;
    // painting it at the widget bounds clipped the shadow against the edges and
    // made the field look pressed into the page.
    const QRectF surface = surfaceRect();
    const QRectF pill = surface.adjusted(0.5, 0.5, -0.5, -0.5);
    Glass::paintShadow(painter, pill, glass, 0.8);
    Glass::paintPanel(painter, pill, glass, 1.0);

    // The ring fades in instead of appearing on the first click, and it fades
    // out again when focus leaves. A ring that blinks is more distracting than
    // no ring at all.
    const qreal target = m_editor->hasFocus() ? 1.0 : 0.0;
    if (!qFuzzyCompare(m_focus + 0.0001, target)) {
        Animation::start(m_focusAnimation, m_focus, target);
    }
    if (m_focus > 0.002) {
        QColor ring(c.accent);
        ring.setAlphaF(ring.alphaF() * m_focus);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(ring, 1.4));
        // The ring is drawn just outside the panel, so it cannot be clipped by
        // the surface's own edge either.
        painter.drawRoundedRect(pill.adjusted(-0.6, -0.6, 0.6, 0.6), m_radius + 0.6,
                                m_radius + 0.6);
    }
    if (underMouse() && m_focus < 0.01) {
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(c.border), 1.0));
        painter.drawRoundedRect(pill, m_radius, m_radius);
    }

    // The editor is a child widget and paints the text after this returns, so
    // these two never share a painter.
    const qreal icon = iconSize();
    const qreal pad = m_hero ? 20.0 : 14.0;
    const QColor iconColor(c.textMuted);

    drawIcon(painter, icons::Shape::Magnifier,
             QRectF(surface.left() + pad, surface.top() + (surface.height() - icon) / 2.0, icon, icon),
             iconColor);

    if (m_trailing != Trailing::None) {
        const bool active = !m_editor->text().isEmpty();
        drawIcon(painter, icons::Shape::ArrowRight, trailingBox(),
                 active ? QColor(c.accent) : iconColor);
    }
}

}  // namespace yozora
