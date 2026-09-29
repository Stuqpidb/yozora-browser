// SPDX-License-Identifier: MIT
#pragma once

#include <QLineEdit>
#include <QString>
#include <QUrl>

namespace yozora {

// The Yozora address bar.
//
// Responsibilities:
//   * show the URL of the active tab in a readable form;
//   * select everything when focused, so typing replaces the address;
//   * classify what the user typed and hand the decision to the shell, which
//     owns the search engine setting.
class AddressBar : public QLineEdit {
    Q_OBJECT

public:
    explicit AddressBar(QWidget* parent = nullptr);

    // Displays `url`. Transient pages (about:blank, the Yozora new tab page,
    // error pages) clear the field instead.
    void displayUrl(const QUrl& url);

    // Puts raw text into the field without selecting it.
    void setRawText(const QString& text);

    // Focuses the field and selects everything (Ctrl+L).
    void focusAndSelectAll();

signals:
    // The user pressed Enter. `isSearch` is true when the shell must run the
    // text through the configured search engine.
    void navigationRequested(const QString& text, bool isSearch);

protected:
    void focusInEvent(QFocusEvent* event) override;

private:
    void submit();

    bool m_internalUpdate = false;
};

}  // namespace yozora
