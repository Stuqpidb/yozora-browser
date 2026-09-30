// SPDX-License-Identifier: MIT
#include "ui/AddressBar.h"

#include "utils/UrlUtils.h"

#include <QFocusEvent>
#include <QFont>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>

namespace yozora {

AddressBar::AddressBar(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText(tr("Search or enter address"));
    setClearButtonEnabled(false);
    // Room for the painted magnifier on the left and the "go" arrow on the right.
    setTextMargins(32, 0, 34, 0);

    connect(this, &QLineEdit::returnPressed, this, &AddressBar::submit);
}

void AddressBar::paintEvent(QPaintEvent* event)
{
    QLineEdit::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QColor muted = palette().color(QPalette::PlaceholderText);
    const QColor accent = palette().color(QPalette::Link);
    const int cy = height() / 2;

    // Leading magnifier.
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(muted, 1.7));
    painter.drawEllipse(QRectF(14, cy - 6.0, 10.5, 10.5));
    painter.drawLine(QPointF(22.5, cy + 2.5), QPointF(27, cy + 7));

    // Trailing "go" arrow.
    QFont arrowFont = font();
    arrowFont.setPointSizeF(13.0);
    painter.setFont(arrowFont);
    painter.setPen(accent);
    painter.drawText(QRect(width() - 36, 0, 24, height()), Qt::AlignCenter,
                     QStringLiteral("\u2192"));
}

void AddressBar::displayUrl(const QUrl& url)
{
    const QString scheme = url.scheme();
    if (scheme == QLatin1String("about") || scheme == QLatin1String("qrc")
        || scheme == QLatin1String("yozora-error") || scheme == QLatin1String("data")) {
        m_internalUpdate = true;
        clear();
        m_internalUpdate = false;
        return;
    }

    m_internalUpdate = true;
    setText(url::toDisplayString(url.toString()));
    setCursorPosition(0);
    m_internalUpdate = false;
}

void AddressBar::setRawText(const QString& text)
{
    m_internalUpdate = true;
    setText(text);
    setCursorPosition(text.size());
    m_internalUpdate = false;
}

void AddressBar::focusAndSelectAll()
{
    setFocus(Qt::ShortcutFocusReason);
    selectAll();
}

void AddressBar::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    if (!m_internalUpdate) {
        selectAll();
    }
}

void AddressBar::submit()
{
    const QString input = text().trimmed();
    const auto kind = url::classify(input);
    if (kind == url::InputKind::Empty) {
        return;
    }
    emit navigationRequested(input, kind == url::InputKind::Search);
}

}  // namespace yozora
