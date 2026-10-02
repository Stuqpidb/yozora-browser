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

class HistoryStore;
class HomePage;
class Settings;
class WebPage;
class WebView;

// One browser tab: either the Yozora start page or a web page. The start page is
// a native widget, so it appears instantly and needs no bridge between C++ and
// the renderer.
class BrowserTab : public QWidget {
    Q_OBJECT

public:
    static constexpr int kHomePageIndex = 0;
    static constexpr int kPageIndex = 1;

    // The pseudo-scheme that means "show the Yozora home page".
    static QUrl startPageUrl();

    BrowserTab(QWebEngineProfile* profile, Settings* settings, HistoryStore* history,
               QWidget* parent = nullptr);
    ~BrowserTab() override;

    // --- navigation -----------------------------------------------------
    void loadUrl(const QUrl& url);
    void loadInput(const QString& text, bool isSearch);
    // Opens a local file the user dropped onto the page. Goes through the same
    // navigation path, with a one-shot file:// permission.
    void openLocalFile(const QUrl& url);
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
    [[nodiscard]] HomePage* homePage() const { return m_home; }

    void updateSearchEngineUi() {}

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
    HomePage* m_home = nullptr;
    WebView* m_view = nullptr;
    Settings* m_settings = nullptr;
    HistoryStore* m_history = nullptr;
    QString m_lastTitle;
    QUrl m_lastUrl;
    // Window state to restore when a page leaves full screen (so a maximized
    // window comes back maximized and a normal one comes back normal).
    Qt::WindowStates m_stateBeforeFullScreen = Qt::WindowNoState;
};

}  // namespace yozora
