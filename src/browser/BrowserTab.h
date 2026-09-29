// SPDX-License-Identifier: MIT
#pragma once

#include <QIcon>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QWidget>

class QStackedWidget;
class QWebEngineProfile;

namespace yozora {

class NewTabPage;
class Settings;
class WebPage;
class WebView;

// One browser tab: either the Yozora start page or a web page.
//
// The start page is a native widget, not a rendered document, so it appears
// instantly and needs no bridge between C++ and the renderer. Both surfaces
// live in a QStackedWidget, which is the only thing the shell has to care
// about.
class BrowserTab : public QWidget {
    Q_OBJECT

public:
    static constexpr int kStartPageIndex = 0;
    static constexpr int kPageIndex = 1;

    // The pseudo-scheme that means "show the Yozora start page".
    static QUrl startPageUrl();

    BrowserTab(QWebEngineProfile* profile, Settings* settings, QWidget* parent = nullptr);
    ~BrowserTab() override;

    // --- navigation -----------------------------------------------------
    void loadUrl(const QUrl& url);
    // Turns raw address-bar input into a URL, running it through the search
    // engine when `isSearch` is true.
    void loadInput(const QString& text, bool isSearch);
    void showStartPage();
    void goBack();
    void goForward();
    void reload();
    void stop();

    // --- state ----------------------------------------------------------
    [[nodiscard]] QUrl url() const;
    [[nodiscard]] QString title() const;
    [[nodiscard]] QIcon icon() const;
    [[nodiscard]] bool canGoBack() const;
    [[nodiscard]] bool canGoForward() const;
    [[nodiscard]] bool isLoading() const;
    [[nodiscard]] int loadProgress() const;
    [[nodiscard]] bool isStartPage() const;

    [[nodiscard]] WebView* view() const { return m_view; }
    [[nodiscard]] NewTabPage* startPage() const { return m_startPage; }

    void setDarkMode(bool dark);
    void updateSearchEngineUi();

    // URLs currently open in this tab, used for session restore.
    [[nodiscard]] QStringList openUrls() const;

signals:
    void titleChanged();
    void urlChanged();
    void loadingChanged(bool loading);
    void loadProgressChanged(int percent);
    void canGoBackChanged(bool canGoBack);
    void canGoForwardChanged(bool canGoForward);
    void newTabRequested(const QUrl& url, bool foreground);
    void statusMessage(const QString& message);

private:
    void updateTitle();
    void updateState();
    void showWebPage();

    QStackedWidget* m_stack = nullptr;
    NewTabPage* m_startPage = nullptr;
    WebView* m_view = nullptr;
    Settings* m_settings = nullptr;
    QString m_lastTitle;
    QUrl m_lastUrl;
    bool m_dark = true;
};

}  // namespace yozora
