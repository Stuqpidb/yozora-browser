// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QPoint>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QProgressBar;
class QVBoxLayout;

namespace yozora {

class AddressBar;
class NavButton;

// Top navigation bar: back / forward / reload on the left, the Yozora address
// field in the middle, then the bookmark star and the menu on the right. Button
// states are driven entirely by the active tab's signals.
class NavigationBar : public QWidget {
    Q_OBJECT

public:
    explicit NavigationBar(QWidget* parent = nullptr);
    ~NavigationBar() override;

    [[nodiscard]] AddressBar* addressBar() const { return m_addressBar; }

    void setCanGoBack(bool can);
    void setCanGoForward(bool can);
    void setLoading(bool loading);
    void setLoadProgress(int percent);

    // Fills or clears the bookmark star for the current page.
    void setBookmarked(bool bookmarked);
    // Shows or hides the "PRIVATE" badge for private browsing windows.
    void setPrivateMode(bool enabled);

    // Repaints every control for the current theme.
    // Shows a transient message next to the address bar.
    void showMessage(const QString& message);
    void showMessage(const QString& message, int timeoutMs);

signals:
    void backRequested();
    void forwardRequested();
    void reloadRequested();
    void stopRequested();
    void bookmarkRequested();
    void menuRequested(const QPoint& globalPos);

private:
    NavButton* makeButton(icons::Shape shape, const QString& tooltip);

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_privateBadge = nullptr;
    NavButton* m_backButton = nullptr;
    NavButton* m_forwardButton = nullptr;
    NavButton* m_reloadButton = nullptr;
    NavButton* m_starButton = nullptr;
    NavButton* m_menuButton = nullptr;

    QWidget* m_addressContainer = nullptr;
    QVBoxLayout* m_addressLayout = nullptr;
    AddressBar* m_addressBar = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_messageLabel = nullptr;

    bool m_loading = false;
};

}  // namespace yozora
