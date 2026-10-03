// SPDX-License-Identifier: MIT
#include "ui/TabStrip.h"

#include "core/Glass.h"
#include "core/Theme.h"
#include "ui/Icons.h"

#include <QContextMenuEvent>
#include <QFontMetrics>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QWheelEvent>

namespace yozora {

namespace {
constexpr int kHeight = 44;
constexpr int kBrandWidth = 134;
constexpr int kTabTop = 8;
constexpr int kTabHeight = 30;
constexpr int kGap = 4;
constexpr int kMinTabWidth = 110;
constexpr int kMaxTabWidth = 220;
constexpr int kCloseSize = 18;
constexpr int kPlusWidth = 34;
constexpr int kPinnedTabWidth = 44;

QColor withAlphaColor(const QColor& color, int alpha)
{
    QColor result = color;
    result.setAlpha(alpha);
    return result;
}
}  // namespace

TabStrip::TabStrip(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFixedHeight(kHeight);
}

void TabStrip::HoverTrack::resize(int count)
{
    m_values.resize(count);
}

void TabStrip::HoverTrack::set(int index, qreal target)
{
    if (index < 0 || index >= m_values.size()) {
        return;
    }
    m_values[index] = qBound(0.0, target, 1.0);
}

qreal TabStrip::HoverTrack::value(int index) const
{
    if (index < 0 || index >= m_values.size()) {
        return 0.0;
    }
    return m_values.at(index);
}

void TabStrip::HoverTrack::advance(qreal step)
{
    for (qreal& value : m_values) {
        if (qAbs(value) < step) {
            value = 0.0;
        } else if (value > 0.0) {
            value -= step;
        } else {
            value += step;
        }
    }
}

bool TabStrip::HoverTrack::atRest() const
{
    for (qreal value : m_values) {
        if (value != 0.0) {
            return false;
        }
    }
    return true;
}

void TabStrip::startHoverAnimation()
{
    // The highlight is stepped towards its target on a timer rather than driven
    // by a QVariantAnimation: the targets change while it runs (the pointer
    // moves to another tab), and a step function handles that without being
    // restarted. 60Hz over 110ms is the same curve the rest of the interface
    // uses, reached with the arithmetic spelled out.
    if (m_hoverAnimating) {
        return;
    }
    m_hoverAnimating = true;

    auto* timer = new QTimer(this);
    timer->setInterval(16);
    connect(timer, &QTimer::timeout, this, [this, timer] {
        m_hoverAmount.advance(1.0 / 6.6);  // 1.0 over ~110ms
        if (m_hoverAmount.atRest()) {
            m_hoverAnimating = false;
            timer->stop();
            timer->deleteLater();
        }
        update();
    });
    timer->start();
}

void TabStrip::setTabs(const QList<Tab>& tabs)
{
    m_tabs = tabs;
    m_hoverAmount.resize(tabs.size());
    if (m_hover >= tabs.size()) {
        m_hover = -1;
    }
    update();
}

void TabStrip::setPinned(int index, bool pinned)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    m_tabs[index].pinned = pinned;
    update();
}

void TabStrip::setCurrentIndex(int index)
{
    if (index == m_current) {
        return;
    }
    m_current = index;
    update();
}

QSize TabStrip::sizeHint() const
{
    return {900, kHeight};
}

QSize TabStrip::minimumSizeHint() const
{
    return {kBrandWidth + 80, kHeight};
}

int TabStrip::pinnedTabWidth() const
{
    return kPinnedTabWidth;
}

int TabStrip::tabWidth() const
{
    const int count = static_cast<int>(m_tabs.size());
    if (count == 0) {
        return 0;
    }
    const int pinnedWidth = pinnedTabWidth();
    int pinnedCount = 0;
    for (const Tab& tab : m_tabs) {
        if (tab.pinned) {
            ++pinnedCount;
        }
    }
    const int unpinned = count - pinnedCount;
    const int reserved = pinnedCount * (pinnedWidth + kGap);
    const int available =
        width() - kBrandWidth - kPlusWidth - 12 - kGap * (count - 1) - reserved;
    if (unpinned <= 0) {
        return 0;
    }
    return qBound(kMinTabWidth, available / unpinned, kMaxTabWidth);
}

QRect TabStrip::tabRect(int index) const
{
    const int pinnedWidth = pinnedTabWidth();
    const int unpinnedWidth = tabWidth();
    int x = kBrandWidth;
    for (int i = 0; i < m_tabs.size(); ++i) {
        const bool pinned = m_tabs.at(i).pinned;
        const int w = pinned ? pinnedWidth : unpinnedWidth;
        if (i == index) {
            return QRect(x, kTabTop, w, kTabHeight);
        }
        x += w + kGap;
    }
    return QRect(x, kTabTop, unpinnedWidth, kTabHeight);
}

QRect TabStrip::closeRect(int index) const
{
    const QRect tab = tabRect(index);
    return QRect(tab.right() - kCloseSize - 6, tab.top() + (tab.height() - kCloseSize) / 2,
                 kCloseSize, kCloseSize);
}

QRect TabStrip::plusRect() const
{
    const QRect last = m_tabs.isEmpty() ? QRect(kBrandWidth, kTabTop, 0, kTabHeight)
                                        : tabRect(static_cast<int>(m_tabs.size()) - 1);
    return QRect(last.right() + kGap + 2, kTabTop + 1, 28, 28);
}

int TabStrip::tabAt(const QPoint& pos) const
{
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (tabRect(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

bool TabStrip::isDragRegion(const QPoint& local) const
{
    return tabAt(local) < 0 && !plusRect().contains(local);
}

int TabStrip::dropIndexFor(const QPoint& pos) const
{
    const int count = static_cast<int>(m_tabs.size());
    // Pinned tabs keep the left end; an unpinned tab can never be dropped before
    // them, so the search starts after the pinned run.
    int firstUnpinned = 0;
    while (firstUnpinned < count && m_tabs.at(firstUnpinned).pinned) {
        ++firstUnpinned;
    }
    for (int i = firstUnpinned; i < count; ++i) {
        const QRect rect = tabRect(i);
        if (pos.x() < rect.center().x()) {
            return i;
        }
    }
    return count - 1;
}

void TabStrip::paintEvent(QPaintEvent*)
{
    const Theme::Colors c = Theme::colors();
    const Glass::Recipe glass = Glass::recipe(11);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // No background here: the tab strip sits on the window's glass top bar, so
    // it must stay transparent and let that surface show through.

    // Brand: the real Yozora mark and the wordmark in the accent gradient. The
    // mark used to be a Unicode four-point star, which is not the product's logo
    // and read as a glyph from some other app.
    QFont brandFont = font();
    brandFont.setPointSizeF(12.5);
    brandFont.setWeight(QFont::DemiBold);
    brandFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.6);
    painter.setFont(brandFont);

    constexpr int kMarkSize = 20;
    const int markY = (height() - kMarkSize) / 2;
    const QRect markRect(16, markY, kMarkSize, kMarkSize);
    const QPixmap mark(QStringLiteral(":/icons/yozora.png"));
    if (!mark.isNull()) {
        painter.drawPixmap(markRect, mark.scaled(markRect.size(), Qt::KeepAspectRatio,
                                                 Qt::SmoothTransformation));
    }

    const QRect brandRect(16 + kMarkSize + 8, 0, kBrandWidth - 20, height());
    QLinearGradient gradient(brandRect.topLeft(), brandRect.topRight());
    gradient.setColorAt(0.0, QColor(c.accent));
    gradient.setColorAt(1.0, QColor(c.accent2));
    painter.setPen(QPen(QBrush(gradient), 1));
    painter.drawText(brandRect, Qt::AlignVCenter | Qt::AlignLeft, QStringLiteral("YOZORA"));

    // Tabs.
    for (int i = 0; i < static_cast<int>(m_tabs.size()); ++i) {
        if (m_dragging && i == m_dragIndex) {
            continue;  // drawn last, under the cursor
        }
        const QRect tab = tabRect(i);
        const bool selected = (i == m_current);
        // Smoothed per tab: the pointer can move several tabs in one frame, and
        // a per-tab fade is what stops the highlight from teleporting between
        // them.
        const qreal hover = qBound(0.0, m_hoverAmount.value(i), 1.0);

        // The active tab is a real glass surface: shadow, translucent body, top
        // highlight, hairline. Inactive tabs stay nearly invisible until the
        // pointer is over them, which keeps the strip calm.
        if (selected) {
            Glass::paintShadow(painter, QRectF(tab), glass, 0.9);
            Glass::paintPanel(painter, QRectF(tab), glass, 1.0);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(c.tabInactive));
            painter.drawRoundedRect(tab, 11, 11);
            if (hover > 0.001) {
                Glass::paintChip(painter, QRectF(tab), glass, hover);
            }
        }

        // A short accent bar under the active tab: the same trick the old
        // design used, kept because it reads at a glance.
        if (selected) {
            const QRect marker(tab.center().x() - 11, tab.bottom() - 2, 22, 2);
            QLinearGradient bar(marker.topLeft(), marker.topRight());
            bar.setColorAt(0.0, withAlphaColor(QColor(c.accent), 0));
            bar.setColorAt(0.5, QColor(c.accent));
            bar.setColorAt(1.0, withAlphaColor(QColor(c.accent), 0));
            painter.setPen(Qt::NoPen);
            painter.setBrush(bar);
            painter.drawRoundedRect(marker, 1, 1);
        }

        const bool pinned = m_tabs.at(i).pinned;

        // Favicon or a placeholder dot. A pinned tab shows only the icon,
        // centred: that is what makes it narrow and recognisable.
        const int iconY = tab.top() + (tab.height() - 16) / 2;
        QRect iconRect(pinned ? tab.center().x() - 8 : tab.left() + 11, iconY, 16, 16);
        const QIcon icon = m_tabs.at(i).icon;
        if (!icon.isNull()) {
            icon.paint(&painter, iconRect, Qt::AlignCenter, QIcon::Normal);
        } else {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(selected ? c.accent : c.textMuted));
            painter.drawEllipse(iconRect.center(), 3, 3);
        }

        if (!pinned) {
            const int textLeft = iconRect.right() + 8;
            const int textRight = tab.right() - kCloseSize - 12;
            QFont tabFont = font();
            tabFont.setPointSizeF(12.0);
            painter.setFont(tabFont);
            painter.setPen(selected ? QColor(c.text) : QColor(c.textMuted));
            const QString title = QFontMetrics(tabFont).elidedText(
                m_tabs.at(i).title, Qt::ElideRight, qMax(10, textRight - textLeft));
            painter.drawText(QRect(textLeft, tab.top(), textRight - textLeft, tab.height()),
                             Qt::AlignVCenter | Qt::AlignLeft, title);
        }

        // Close button. Pinned tabs have none: the icon is the whole tab.
        if (!pinned && (selected || hover > 0.01)) {
            const QRect close = closeRect(i);
            if (hover > 0.5 && m_closeHover) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(c.danger));
                painter.drawRoundedRect(close, 6, 6);
                painter.setPen(QColor(c.accentText));
            } else {
                painter.setPen(QColor(c.textMuted));
            }
            QFont closeFont = font();
            closeFont.setPointSizeF(9.0);
            painter.setFont(closeFont);
            icons::draw(painter, icons::Shape::Close, QRectF(close).adjusted(3, 3, -3, -3),
                        QColor(c.textMuted), 1.6);
        }
    }

    // Fallback drop indicator while dragging.
    if (m_dragging && m_dragIndex >= 0) {
        const QRect tab = tabRect(m_dragIndex);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(c.accent));
        painter.drawRoundedRect(QRect(tab.left(), height() - 3, tab.width(), 2), 1, 1);
    }

    // "+" button.
    const QRect plus = plusRect();
    if (m_plusHover) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(c.surfaceHover));
        painter.drawRoundedRect(plus, 9, 9);
    }
    QFont plusFont = font();
    plusFont.setPointSizeF(15.0);
    painter.setFont(plusFont);
    painter.setPen(QColor(m_plusHover ? c.text : c.textMuted));
    painter.drawText(plus, Qt::AlignCenter, QStringLiteral("+"));
}

void TabStrip::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    const QPoint pos = event->position().toPoint();

    if (plusRect().contains(pos)) {
        emit newTabRequested();
        return;
    }

    const int index = tabAt(pos);
    if (index < 0) {
        return;
    }

    if (!m_tabs.at(index).pinned && closeRect(index).contains(pos)
        && (index == m_current || index == m_hover)) {
        emit closeRequested(index);
        return;
    }

    emit currentChanged(index);
    m_pressed = true;
    m_pressIndex = index;
    m_pressPos = pos;
}

void TabStrip::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    const int index = tabAt(pos);

    bool changed = false;
    if (index != m_hover) {
        m_hover = index;
        // The tab the pointer left retracts, the one it arrived at fills.
        m_hoverAmount.set(m_hover, 1.0);
        startHoverAnimation();
        changed = true;
    }
    const bool closeHover = index >= 0 && closeRect(index).contains(pos);
    if (closeHover != m_closeHover) {
        m_closeHover = closeHover;
        changed = true;
    }
    const bool plusHover = plusRect().contains(pos);
    if (plusHover != m_plusHover) {
        m_plusHover = plusHover;
        changed = true;
    }

    if (m_pressed && (pos - m_pressPos).manhattanLength() > 8
        && !m_tabs.at(qBound(0, m_pressIndex, static_cast<int>(m_tabs.size()) - 1)).pinned) {
        // A pinned tab is not draggable: it is pinned to its place by definition.
        m_dragging = true;
        m_dragIndex = m_pressIndex;
    }
    if (m_dragging) {
        m_dragPos = pos;
        changed = true;
    }

    if (changed) {
        update();
    }
}

void TabStrip::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (m_dragging && m_dragIndex >= 0) {
        const int target = dropIndexFor(event->position().toPoint());
        if (target >= 0 && target != m_dragIndex) {
            emit moveRequested(m_dragIndex, target);
        }
    }
    m_pressed = false;
    m_dragging = false;
    m_dragIndex = -1;
    update();
}

void TabStrip::leaveEvent(QEvent*)
{
    m_hover = -1;
    m_hoverAmount.set(m_hover, 0.0);
    startHoverAnimation();
    m_closeHover = false;
    m_plusHover = false;
    update();
}

void TabStrip::wheelEvent(QWheelEvent* event)
{
    const int direction = event->angleDelta().y() > 0 ? -1 : 1;
    const int next = m_current + direction;
    if (next >= 0 && next < static_cast<int>(m_tabs.size())) {
        emit currentChanged(next);
    }
    event->accept();
}

void TabStrip::contextMenuEvent(QContextMenuEvent* event)
{
    const int index = tabAt(event->pos());
    if (index >= 0) {
        emit contextMenuRequested(index, event->globalPos());
    }
    event->accept();
}

}  // namespace yozora
