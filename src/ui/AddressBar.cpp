// SPDX-License-Identifier: MIT
#include "ui/AddressBar.h"

#include "utils/UrlUtils.h"

#include <QFocusEvent>

namespace yozora {

AddressBar::AddressBar(QWidget* parent)
    : QLineEdit(parent)
{
    setPlaceholderText(tr("Search or enter address"));
    setClearButtonEnabled(true);

    connect(this, &QLineEdit::returnPressed, this, &AddressBar::submit);
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
