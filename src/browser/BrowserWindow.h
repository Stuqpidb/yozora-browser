// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QMainWindow>
#include <QPoint>
#include <QPointer>
#include <QStringList>
#include <QUrl>

class QAction;
class QShortcut;

namespace yozora {

class BrowserTab;
class DownloadManager;
class NavigationBar;
class Settings;
class TabWidget;
class UpdateChecker;
class WebProfile;

// The Yozora main window: navigation bar, tab strip, page area and the
// download strip. All shortcuts live here, so the whole key map of the browser
// can be read in one place.
class BrowserWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BrowserWindow(WebProfile* profile, Settings* settings, QWidget* parent = nullptr);
    ~BrowserWindow() override;

    // --- tabs -------------------------------------------------------------
    BrowserTab* newTab(const QUrl& url = QUrl(), bool foreground = true);
    // Navigates the first tab, replacing the start page it currently shows.
    void openInFirstTab(const QUrl& url);
    void closeTab(int index);
    void closeCurrentTab();
    void restoreLastClosedTab();
    [[nodiscard]] int tabCount() const;
    [[nodiscard]] BrowserTab* currentTab() const;

    // --- navigation -------------------------------------------------------
    void goBack();
    void goForward();
    void reload();
    void stop();
    void focusAddressBar();
    void openDevTools();

    // Turns address-bar input into a navigation, searching when asked to.
    void navigateInput(const QString& text, bool isSearch);

    // --- window -----------------------------------------------------------
    void openNewWindow();
    void showSettings();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct SessionSnapshot {
        QStringList urls;
        int activeIndex = 0;
    };

    void buildUi();
    void buildShortcuts();
    void buildMenu(const QPoint& globalPos);
    void connectTab(BrowserTab* tab);
    void updateForActiveTab();
    void updateTabLabel(int index);
    void showStatusMessage(const QString& message);
    SessionSnapshot snapshot() const;
    void restoreSnapshot(const SessionSnapshot& snap);
    void applyTheme(bool dark);
    [[nodiscard]] bool isDark() const;

    WebProfile* m_profile = nullptr;
    Settings* m_settings = nullptr;
    UpdateChecker* m_updateChecker = nullptr;

    NavigationBar* m_navBar = nullptr;
    TabWidget* m_tabWidget = nullptr;
    DownloadManager* m_downloads = nullptr;

    QList<SessionSnapshot> m_closedTabs;
    QList<QPointer<QWidget>> m_devToolsWindows;
    bool m_closing = false;
};

}  // namespace yozora
