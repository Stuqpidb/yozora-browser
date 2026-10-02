// SPDX-License-Identifier: MIT
#pragma once

#include "privacy/FilterEngine.h"
#include "privacy/RequestInterceptor.h"

#include <QList>
#include <QMainWindow>
#include <QPoint>
#include <QPointer>
#include <QStringList>
#include <QUrl>

class QAction;
class QShortcut;
class QStackedWidget;

namespace yozora {

class BookmarkStore;
class BrowserTab;
class DownloadManager;
class HistoryStore;
class NavigationBar;
class PermissionManager;
class Settings;
class ShieldDialog;
class SideBar;
class TabStrip;
class UpdateChecker;
class UpdateDialog;
class WebProfile;
struct ReleaseInfo;

// The Yozora main window. Its chrome is fully custom: a left navigation rail, a
// custom tab strip and a navigation bar, with a stack of page views behind them.
// All shortcuts live here, so the whole key map can be read in one place.
class BrowserWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BrowserWindow(WebProfile* profile, Settings* settings, BookmarkStore* bookmarks,
                           HistoryStore* history, bool privateMode = false,
                           QWidget* parent = nullptr);
    ~BrowserWindow() override;

    // --- tabs -------------------------------------------------------------
    BrowserTab* newTab(const QUrl& url = QUrl(), bool foreground = true);
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

    void navigateInput(const QString& text, bool isSearch);

    // --- window -----------------------------------------------------------
    void openNewWindow();
    void openPrivateWindow();
    void showSettings();
    void showClearBrowsingData();
    void showShield();

    [[nodiscard]] bool isPrivateMode() const { return m_private; }

protected:
    void closeEvent(QCloseEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    struct SessionSnapshot {
        QStringList urls;
        int activeIndex = 0;
    };

    void buildUi();
    void buildShortcuts();
    void buildMenu(const QPoint& globalPos);
    void connectTab(BrowserTab* tab);
    void selectTab(int index);
    void refreshTabStrip();
    void updateForActiveTab();
    void updateTabLabel(int index);
    void showStatusMessage(const QString& message);
    // Update flow. The window owns the checker and the dialog; the checker owns
    // the network, and the dialog never fetches or launches anything itself.
    void showUpdateOffer(const ReleaseInfo& info);
    void showUpdateProblem(const QString& reason);
    void handleExternalProtocol(const QUrl& url, int navigationType);
    void toggleBookmark();
    void updateBookmarkStar();
    void updateShieldState();
    void showLibrary(bool bookmarks);
    SessionSnapshot snapshot() const;
    void restoreSnapshot(const SessionSnapshot& snap);

    WebProfile* m_profile = nullptr;
    Settings* m_settings = nullptr;
    BookmarkStore* m_bookmarks = nullptr;
    HistoryStore* m_history = nullptr;
    UpdateChecker* m_updateChecker = nullptr;
    QPointer<UpdateDialog> m_updateDialog;
    QPointer<ShieldDialog> m_shieldDialog;
    PermissionManager* m_permissions = nullptr;
    BlockingStats m_blockingStats;

    SideBar* m_sideBar = nullptr;
    TabStrip* m_tabStrip = nullptr;
    NavigationBar* m_navBar = nullptr;
    QStackedWidget* m_pages = nullptr;
    DownloadManager* m_downloads = nullptr;

    QList<BrowserTab*> m_tabs;
    QList<SessionSnapshot> m_closedTabs;
    QList<QPointer<QWidget>> m_devToolsWindows;
    int m_lastActiveIndex = 0;
    bool m_private = false;
    bool m_closing = false;
};

}  // namespace yozora
