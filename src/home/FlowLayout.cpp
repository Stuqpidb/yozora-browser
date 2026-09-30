// SPDX-License-Identifier: MIT
#include "home/FlowLayout.h"

#include <QStyle>
#include <QWidget>
#include <QtGlobal>

FlowLayout::FlowLayout(QWidget* parent, int margin, int horizontalSpacing, int verticalSpacing)
    : QLayout(parent)
    , m_horizontalSpacing(horizontalSpacing)
    , m_verticalSpacing(verticalSpacing)
{
    if (margin >= 0) {
        setContentsMargins(margin, margin, margin, margin);
    }
}

FlowLayout::~FlowLayout()
{
    while (QLayoutItem* item = takeAt(0)) {
        delete item;
    }
}

void FlowLayout::addItem(QLayoutItem* item)
{
    m_items.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    if (m_horizontalSpacing >= 0) {
        return m_horizontalSpacing;
    }
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (m_verticalSpacing >= 0) {
        return m_verticalSpacing;
    }
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

void FlowLayout::setHorizontalSpacing(int spacing)
{
    m_horizontalSpacing = spacing;
    invalidate();
}

void FlowLayout::setVerticalSpacing(int spacing)
{
    m_verticalSpacing = spacing;
    invalidate();
}

int FlowLayout::count() const
{
    return static_cast<int>(m_items.size());
}

QLayoutItem* FlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem* FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }
    return m_items.takeAt(index);
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return Qt::Orientations();
}

bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), false);
}

void FlowLayout::setGeometry(const QRect& rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, true);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    for (const QLayoutItem* item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }
    const QMargins margins = contentsMargins();
    return size + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

int FlowLayout::doLayout(const QRect& rect, bool apply) const
{
    const QMargins margins = contentsMargins();
    const QRect effective = rect.adjusted(margins.left(), margins.top(), -margins.right(),
                                          -margins.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;

    for (QLayoutItem* item : m_items) {
        const QWidget* widget = item->widget();
        if (widget && widget->isHidden()) {
            continue;
        }
        const QSize hint = item->sizeHint();
        int next = x + hint.width() + horizontalSpacing();
        // Wrapping, not squeezing: if the item does not fit on this line it
        // moves down instead of being compressed until its text is unreadable.
        if (next - horizontalSpacing() > effective.right() + 1 && lineHeight > 0) {
            x = effective.x();
            y = y + lineHeight + verticalSpacing();
            next = x + hint.width() + horizontalSpacing();
            lineHeight = 0;
        }
        if (apply) {
            item->setGeometry(QRect(QPoint(x, y), hint));
        }
        x = next;
        lineHeight = qMax(lineHeight, hint.height());
    }

    return y + lineHeight - rect.y() + margins.bottom();
}

int FlowLayout::smartSpacing(QStyle::PixelMetric metric) const
{
    QObject* parent = this->parent();
    if (!parent) {
        return -1;
    }
    if (parent->isWidgetType()) {
        auto* widget = static_cast<QWidget*>(parent);
        return widget->style()->pixelMetric(metric, nullptr, widget);
    }
    return static_cast<QLayout*>(parent)->spacing();
}
