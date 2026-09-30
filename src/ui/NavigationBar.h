// SPDX-License-Identifier: MIT
#pragma once

#include <QPoint>
#include <QWidget>

class QHBoxLayout;
class QLabel;
class QProgressBar;
class QToolButton;
class QVBoxLayout;

namespace yozora {

class AddressBar;

// Top navigation bar: back / forward / reload on the left, the Yozora address
// pill in the middle, then the bookmark star and the menu on the right. Button
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
    QToolButton* makeButton(const QString& objectName, const QString& glyph,
                            const QString& tooltip);

    QHBoxLayout* m_layout = nullptr;
    QLabel* m_privateBadge = nullptr;
    QToolButton* m_backButton = nullptr;
    QToolButton* m_forwardButton = nullptr;
    QToolButton* m_reloadButton = nullptr;
    QToolButton* m_starButton = nullptr;
    QToolButton* m_menuButton = nullptr;

    QWidget* m_addressContainer = nullptr;
    QVBoxLayout* m_addressLayout = nullptr;
    AddressBar* m_addressBar = nullptr;
    QProgressBar* m_progress = nullptr;
    QLabel* m_messageLabel = nullptr;

    bool m_loading = false;
};

}  // namespace yozora
