// SPDX-License-Identifier: MIT
#include "browser/BrowserWindow.h"

#include "browser/BrowserTab.h"
#include "core/BookmarkStore.h"
#include "core/Glass.h"
#include "core/HistoryStore.h"
#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "core/Theme.h"
#include "core/UpdateChecker.h"
#include "home/HomePage.h"
#include "privacy/PermissionManager.h"
#include "ui/AddressBar.h"
#include "ui/ClearBrowsingDataDialog.h"
#include "ui/LibraryDialog.h"
#include "ui/NavigationBar.h"
#include "ui/SettingsDialog.h"
#include "ui/SideBar.h"
#include "ui/TabStrip.h"
#include "ui/UpdateDialog.h"
#include "utils/UrlUtils.h"
#include "utils/Version.h"
#include "web/DownloadManager.h"
#include "web/WebPage.h"
#include "web/WebProfile.h"
#include "web/WebView.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QShortcut>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace yozora {

namespace {
constexpr int kMaxClosedTabs = 12;
}

BrowserWindow::BrowserWindow(WebProfile* profile, Settings* settings, BookmarkStore* bookmarks,
                             HistoryStore* history, bool privateMode, QWidget* parent)
    : QMainWindow(parent)
    , m_profile(profile)
    , m_settings(settings)
    , m_bookmarks(bookmarks)
    , m_history(history)
    , m_updateChecker(new UpdateChecker(this))
    , m_private(privateMode)
{
    buildUi();
    buildShortcuts();

    m_permissions = new PermissionManager(m_profile->profile(), m_settings, this);
    connect(m_permissions, &PermissionManager::permissionDenied, this,
            [this](const QString& origin, const QString& feature) {
                showStatusMessage(tr("Blocked %1 request from %2").arg(feature, origin));
            });

    if (m_private) {
        setWindowTitle(tr("Yozora - Private Browsing"));
        m_navBar->setPrivateMode(true);
    }

    connect(m_settings, &Settings::searchEngineChanged, this, [this] {
        if (auto* tab = currentTab()) {
            tab->updateSearchEngineUi();
        }
    });

    newTab();

    const QByteArray geometry = m_settings->windowGeometry();
    if (!m_private && !geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        resize(1320, 860);
    }
}

BrowserWindow::~BrowserWindow() = default;

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void BrowserWindow::buildUi()
{
    setObjectName(QStringLiteral("browserWindow"));

    auto* central = new QWidget(this);
    auto* root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    m_sideBar = new SideBar(central);
    root->addWidget(m_sideBar);

    auto* right = new QWidget(central);
    auto* column = new QVBoxLayout(right);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(0);

    m_tabStrip = new TabStrip(right);
    m_navBar = new NavigationBar(right);
    m_navBar->setObjectName(QStringLiteral("navigationBar"));

    m_pages = new QStackedWidget(right);
    m_downloads = new DownloadManager(m_profile->profile(), m_settings, right);

    column->addWidget(m_tabStrip);
    column->addWidget(m_navBar);
    column->addWidget(m_pages, 1);
    column->addWidget(m_downloads->statusBar());
    root->addWidget(right, 1);

    setCentralWidget(central);
    statusBar()->hide();

    connect(m_sideBar, &SideBar::homeRequested, this, [this] {
        if (auto* tab = currentTab()) {
            tab->showStartPage();
        }
        updateForActiveTab();
    });
    connect(m_sideBar, &SideBar::historyRequested, this, [this] { showLibrary(false); });
    connect(m_sideBar, &SideBar::bookmarksRequested, this, [this] { showLibrary(true); });
    connect(m_sideBar, &SideBar::downloadsRequested, this, [this] {
        QDesktopServices::openUrl(QUrl::fromLocalFile(m_settings->downloadDirectory()));
    });
    connect(m_sideBar, &SideBar::privateRequested, this, &BrowserWindow::openPrivateWindow);
    connect(m_sideBar, &SideBar::settingsRequested, this, &BrowserWindow::showSettings);
    // A rail the user hid stays hidden, and a rail they brought back stays
    // visible, for the rest of the session and the next start.
    connect(m_sideBar, &SideBar::collapsedChanged, this, [this](bool collapsed) {
        m_settings->setSideBarCollapsed(collapsed);
        update();
    });
    if (m_settings->sideBarCollapsed()) {
        m_sideBar->setCollapsed(true, false);
    }

    // The update signals are wired once, here, rather than inside buildMenu().
    // The menu is rebuilt every time it is opened, and connecting there added one
    // more connection per opening - after a dozen openings a single check would
    // report its result a dozen times.
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
            [this](const QString& reason, ReleaseError) {
                // A failure is worth a dialog: the usual causes (no network, no
                // public release) need a sentence, not a line in a status strip
                // that is gone before it has been read.
                showUpdateProblem(reason);
            });
    connect(m_updateChecker, &UpdateChecker::updateCheckFinished, this, [this](bool hasUpdate) {
        if (!hasUpdate) {
            showStatusMessage(tr("Yozora is up to date"));
        }
    });
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [this](const ReleaseInfo& info) { showUpdateOffer(info); });
    connect(m_updateChecker, &UpdateChecker::updateDownloadStarted, this,
            [this](const QString& fileName, qint64 total) {
                m_updateDialog->beginDownload(fileName, total);
            });
    connect(m_updateChecker, &UpdateChecker::updateDownloadProgress, this,
            [this](qint64 received, qint64 total) {
                m_updateDialog->setProgress(received, total);
            });
    connect(m_updateChecker, &UpdateChecker::updateDownloadFinished, this,
            [this](const QString& path, bool sizeMatched) {
                m_updateDialog->finishDownload(path, sizeMatched);
            });
    connect(m_updateChecker, &UpdateChecker::updateDownloadFailed, this,
            [this](const QString& reason) { m_updateDialog->failDownload(reason); });

    connect(m_tabStrip, &TabStrip::currentChanged, this, &BrowserWindow::selectTab);
    connect(m_tabStrip, &TabStrip::closeRequested, this, &BrowserWindow::closeTab);
    connect(m_tabStrip, &TabStrip::newTabRequested, this, [this] { newTab(); });
    connect(m_tabStrip, &TabStrip::moveRequested, this, [this](int from, int to) {
        BrowserTab* current = currentTab();
        m_tabs.move(from, to);
        refreshTabStrip();
        if (current) {
            selectTab(m_tabs.indexOf(current));
        }
    });

    connect(m_navBar, &NavigationBar::backRequested, this, &BrowserWindow::goBack);
    connect(m_navBar, &NavigationBar::forwardRequested, this, &BrowserWindow::goForward);
    connect(m_navBar, &NavigationBar::reloadRequested, this, &BrowserWindow::reload);
    connect(m_navBar, &NavigationBar::stopRequested, this, &BrowserWindow::stop);
    connect(m_navBar, &NavigationBar::menuRequested, this, &BrowserWindow::buildMenu);
    connect(m_navBar, &NavigationBar::bookmarkRequested, this, &BrowserWindow::toggleBookmark);
    connect(m_navBar->addressBar(), &AddressBar::navigationRequested, this,
            &BrowserWindow::navigateInput);

    connect(m_downloads, &DownloadManager::downloadStarted, this,
            [this](const QString& name) { showStatusMessage(tr("Downloading %1").arg(name)); });
    connect(m_downloads, &DownloadManager::downloadFinished, this,
            [this](const QString& path) {
                showStatusMessage(tr("Downloaded %1").arg(QFileInfo(path).fileName()));
            });
    connect(m_downloads, &DownloadManager::downloadFailed, this,
            [this](const QString& name, const QString&) {
                showStatusMessage(tr("Download of %1 failed").arg(name));
            });
}

void BrowserWindow::buildShortcuts()
{
    const auto add = [this](const QKeySequence& sequence, auto handler) {
        auto* shortcut = new QShortcut(sequence, this);
        shortcut->setContext(Qt::WindowShortcut);
        connect(shortcut, &QShortcut::activated, this, handler);
    };
    const auto sequence = [](const char* text) { return QKeySequence(QString::fromLatin1(text)); };

    add(sequence("Ctrl+T"), [this] { newTab(); });
    add(sequence("Ctrl+W"), [this] { closeCurrentTab(); });
    add(sequence("Ctrl+Shift+T"), [this] { restoreLastClosedTab(); });
    add(sequence("Ctrl+N"), [this] { openNewWindow(); });
    add(sequence("Ctrl+Shift+N"), [this] { openPrivateWindow(); });

    for (int i = 1; i <= 9; ++i) {
        add(sequence(QStringLiteral("Ctrl+%1").arg(i).toLatin1().constData()), [this, i] {
            const int index = i - 1;
            if (index < m_tabs.size()) {
                selectTab(index);
            } else if (i == 9 && !m_tabs.isEmpty()) {
                selectTab(m_tabs.size() - 1);
            }
        });
    }

    add(sequence("Alt+Left"), [this] { goBack(); });
    add(sequence("Alt+Right"), [this] { goForward(); });
    add(sequence("Ctrl+R"), [this] { reload(); });
    add(sequence("F5"), [this] { reload(); });
    add(sequence("Esc"), [this] {
        if (m_navBar->addressBar()->hasFocus()) {
            m_navBar->addressBar()->clear();
            if (auto* tab = currentTab()) {
                tab->view()->setFocus();
            }
        } else {
            stop();
        }
    });

    add(sequence("Ctrl+L"), [this] { focusAddressBar(); });
    // Ctrl+B hides the rail, the way every other browser does it. When the rail
    // is hidden there is nothing on screen to click, so the shortcut is also the
    // only way back.
    add(sequence("Ctrl+B"), [this] { m_sideBar->setCollapsed(!m_sideBar->isCollapsed()); });
    add(sequence("Ctrl+D"), [this] { toggleBookmark(); });
    add(sequence("Ctrl+Shift+H"), [this] {
        if (auto* tab = currentTab()) {
            tab->showStartPage();
            tab->homePage()->focusSearch();
        }
        updateForActiveTab();
    });
    add(sequence("Ctrl+H"), [this] { showLibrary(false); });
    add(sequence("Ctrl+Shift+O"), [this] { showLibrary(true); });

    add(sequence("Ctrl+Shift+I"), [this] { openDevTools(); });
    add(sequence("F12"), [this] { openDevTools(); });
    add(sequence("Ctrl+,"), [this] { showSettings(); });
    add(sequence("Ctrl+Q"), [this] { close(); });

    const auto zoomBy = [this](double factor) {
        if (auto* tab = currentTab()) {
            if (tab->view()) {
                tab->view()->setZoomFactor(tab->view()->zoomFactor() * factor);
            }
        }
    };
    add(sequence("Ctrl++"), [this, zoomBy] { zoomBy(1.1); });
    add(sequence("Ctrl+-"), [this, zoomBy] { zoomBy(1.0 / 1.1); });
    add(sequence("Ctrl+0"), [this] {
        if (auto* tab = currentTab()) {
            if (tab->view()) {
                tab->view()->setZoomFactor(1.0);
            }
        }
    });
}

// ---------------------------------------------------------------------------
// Tabs
// ---------------------------------------------------------------------------

BrowserTab* BrowserWindow::newTab(const QUrl& url, bool foreground)
{
    auto* tab = new BrowserTab(m_profile->profile(), m_settings, m_history, this);
    connectTab(tab);
    m_pages->addWidget(tab);
    m_tabs.append(tab);

    if (url.isValid() && !url.isEmpty()) {
        tab->loadUrl(url);
    } else {
        tab->showStartPage();
    }

    const int index = m_tabs.size() - 1;
    refreshTabStrip();
    if (foreground) {
        selectTab(index);
        m_navBar->addressBar()->setFocus();
    } else if (m_tabs.size() == 1) {
        selectTab(0);
    }
    return tab;
}

int BrowserWindow::tabCount() const
{
    return static_cast<int>(m_tabs.size());
}

BrowserTab* BrowserWindow::currentTab() const
{
    const int index = m_tabStrip->currentIndex();
    if (index < 0 || index >= m_tabs.size()) {
        return nullptr;
    }
    return m_tabs.at(index);
}

void BrowserWindow::selectTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    m_tabStrip->setCurrentIndex(index);
    m_pages->setCurrentWidget(m_tabs.at(index));
    m_lastActiveIndex = index;
    updateForActiveTab();
}

void BrowserWindow::openInFirstTab(const QUrl& url)
{
    if (m_tabs.size() != 1) {
        newTab(url);
        return;
    }
    if (auto* tab = currentTab()) {
        tab->loadUrl(url);
    }
}

void BrowserWindow::connectTab(BrowserTab* tab)
{
    connect(tab, &BrowserTab::urlChanged, this, &BrowserWindow::updateForActiveTab);
    connect(tab, &BrowserTab::titleChanged, this, [this] { refreshTabStrip(); updateForActiveTab(); });
    connect(tab, &BrowserTab::loadingChanged, this, [this, tab](bool) {
        if (tab == currentTab()) {
            updateForActiveTab();
        }
    });
    connect(tab, &BrowserTab::loadProgressChanged, this, [this, tab](int percent) {
        if (tab == currentTab()) {
            m_navBar->setLoadProgress(percent);
        }
    });
    connect(tab, &BrowserTab::canGoBackChanged, this, [this, tab](bool) {
        if (tab == currentTab()) {
            m_navBar->setCanGoBack(tab->canGoBack());
        }
    });
    connect(tab, &BrowserTab::canGoForwardChanged, this, [this, tab](bool) {
        if (tab == currentTab()) {
            m_navBar->setCanGoForward(tab->canGoForward());
        }
    });
    connect(tab, &BrowserTab::newTabRequested, this,
            [this](const QUrl& url, bool foreground) { newTab(url, foreground); });
    connect(tab, &BrowserTab::statusMessage, this, &BrowserWindow::showStatusMessage);

    if (auto* view = tab->view()) {
        if (auto* page = qobject_cast<WebPage*>(view->page())) {
            if (m_permissions) {
                m_permissions->attachPage(page);
            }
            connect(page, &WebPage::externalProtocolRequested, this,
                    &BrowserWindow::handleExternalProtocol);
        }
    }
}

void BrowserWindow::refreshTabStrip()
{
    QList<TabStrip::Tab> stripTabs;
    stripTabs.reserve(m_tabs.size());
    for (BrowserTab* tab : m_tabs) {
        TabStrip::Tab entry;
        entry.title = tab->title();
        entry.icon = tab->icon();
        entry.tooltip = tab->url().toString();
        stripTabs.append(entry);
    }
    m_tabStrip->setTabs(stripTabs);
    const int current = m_tabs.isEmpty() ? -1 : qBound(0, m_lastActiveIndex, static_cast<int>(m_tabs.size()) - 1);
    m_tabStrip->setCurrentIndex(current);
}

void BrowserWindow::closeTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }

    m_closedTabs.prepend(snapshot());
    while (m_closedTabs.size() > kMaxClosedTabs) {
        m_closedTabs.removeLast();
    }

    BrowserTab* tab = m_tabs.takeAt(index);
    m_pages->removeWidget(tab);
    tab->deleteLater();

    if (m_tabs.isEmpty()) {
        if (m_closing) {
            refreshTabStrip();
            return;
        }
        close();
        return;
    }

    const int next = qBound(0, index, static_cast<int>(m_tabs.size()) - 1);
    refreshTabStrip();
    selectTab(next);
}

void BrowserWindow::closeCurrentTab()
{
    closeTab(m_tabStrip->currentIndex());
}

void BrowserWindow::restoreLastClosedTab()
{
    if (m_closedTabs.isEmpty()) {
        return;
    }
    restoreSnapshot(m_closedTabs.takeFirst());
}

BrowserWindow::SessionSnapshot BrowserWindow::snapshot() const
{
    SessionSnapshot snap;
    for (BrowserTab* tab : m_tabs) {
        snap.urls += tab->openUrls();
    }
    snap.activeIndex = m_tabStrip->currentIndex();
    return snap;
}

void BrowserWindow::restoreSnapshot(const SessionSnapshot& snap)
{
    if (snap.urls.isEmpty()) {
        newTab();
        return;
    }
    for (BrowserTab* tab : m_tabs) {
        m_pages->removeWidget(tab);
        tab->deleteLater();
    }
    m_tabs.clear();

    BrowserTab* active = nullptr;
    for (int i = 0; i < snap.urls.size(); ++i) {
        BrowserTab* tab = newTab(QUrl(snap.urls.at(i)), false);
        if (i == snap.activeIndex) {
            active = tab;
        }
    }
    refreshTabStrip();
    selectTab(active ? m_tabs.indexOf(active) : 0);
}

// ---------------------------------------------------------------------------
// Navigation
// ---------------------------------------------------------------------------

void BrowserWindow::navigateInput(const QString& text, bool isSearch)
{
    if (auto* tab = currentTab()) {
        tab->loadInput(text, isSearch);
    }
}

void BrowserWindow::goBack()
{
    if (auto* tab = currentTab()) {
        tab->goBack();
    }
}

void BrowserWindow::goForward()
{
    if (auto* tab = currentTab()) {
        tab->goForward();
    }
}

void BrowserWindow::reload()
{
    if (auto* tab = currentTab()) {
        tab->reload();
    }
}

void BrowserWindow::stop()
{
    if (auto* tab = currentTab()) {
        tab->stop();
    }
}

void BrowserWindow::focusAddressBar()
{
    m_navBar->addressBar()->focusAndSelectAll();
}

void BrowserWindow::openDevTools()
{
    auto* tab = currentTab();
    if (!tab || !tab->view() || !tab->view()->page()) {
        return;
    }

    auto* devToolsPage = new QWebEnginePage(tab->view()->page()->profile(), this);
    auto* devToolsView = new QWebEngineView;
    devToolsView->setPage(devToolsPage);
    devToolsView->setAttribute(Qt::WA_DeleteOnClose);
    devToolsView->resize(1000, 700);
    devToolsView->setWindowTitle(tr("Yozora DevTools"));
    devToolsPage->setInspectedPage(tab->view()->page());
    devToolsView->show();

    m_devToolsWindows.append(QPointer<QWidget>(devToolsView));
}

// ---------------------------------------------------------------------------
// Window
// ---------------------------------------------------------------------------

void BrowserWindow::openNewWindow()
{
    auto* window = new BrowserWindow(m_profile, m_settings, m_bookmarks, m_history);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->resize(1180, 780);
    window->show();
}

void BrowserWindow::openPrivateWindow()
{
    auto* profile = WebProfile::createEphemeral(m_settings);
    auto* window = new BrowserWindow(profile, m_settings, m_bookmarks, m_history, true);
    profile->setParent(window);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->resize(1180, 780);
    window->show();
}

void BrowserWindow::handleExternalProtocol(const QUrl& url, int navigationType)
{
    Q_UNUSED(navigationType)
    const auto answer = QMessageBox::question(
        this, tr("Open with another application?"),
        tr("This link wants to open an external application.\n\n"
           "Address: %1\nScheme: %2\n\nOpen it?")
            .arg(url.toDisplayString(), url.scheme()),
        QMessageBox::Open | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer == QMessageBox::Open) {
        QDesktopServices::openUrl(url);
    }
}

void BrowserWindow::showSettings()
{
    SettingsDialog dialog(m_settings, m_profile, this);
    connect(&dialog, &SettingsDialog::updateCheckRequested, this, [this, &dialog] {
        m_updateChecker->checkNow();
    });
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
            [&dialog](const QString& reason, ReleaseError) {
                dialog.setUpdateCheckResult(reason, true);
            });
    connect(m_updateChecker, &UpdateChecker::updateCheckFinished, this, [&dialog](bool hasUpdate) {
        if (!hasUpdate) {
            dialog.setUpdateCheckResult(tr("Yozora is up to date."), false);
        }
    });
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [&dialog](const ReleaseInfo& info) {
                dialog.setUpdateCheckResult(
                    tr("Yozora %1 is available.").arg(info.version.toString()), false);
            });
    dialog.exec();
    if (auto* tab = currentTab()) {
        tab->updateSearchEngineUi();
    }
}

void BrowserWindow::showClearBrowsingData()
{
    ClearBrowsingDataDialog dialog(m_profile, this);
    dialog.exec();
    if (auto* tab = currentTab()) {
        tab->reload();
    }
}

void BrowserWindow::toggleBookmark()
{
    auto* tab = currentTab();
    if (!tab || !m_bookmarks) {
        return;
    }
    const QUrl url = tab->url();
    if (!url.isValid() || url.scheme() == QLatin1String("about")) {
        return;
    }
    m_bookmarks->toggle(url.toString(), tab->title());
    updateBookmarkStar();
}

void BrowserWindow::updateBookmarkStar()
{
    auto* tab = currentTab();
    if (!tab || !m_bookmarks) {
        m_navBar->setBookmarked(false);
        return;
    }
    const QUrl url = tab->url();
    m_navBar->setBookmarked(url.isValid() && m_bookmarks->contains(url.toString()));
}

void BrowserWindow::showLibrary(bool bookmarks)
{
    LibraryDialog dialog(bookmarks, m_bookmarks, m_history, this);
    dialog.exec();

    if (auto* tab = currentTab()) {
        if (const QUrl url = dialog.chosenUrl(); url.isValid()) {
            tab->loadUrl(url);
        }
    }
}

void BrowserWindow::showUpdateOffer(const ReleaseInfo& info)
{
    if (!m_updateDialog) {
        m_updateDialog = new UpdateDialog(this);
        m_updateDialog->setAttribute(Qt::WA_DeleteOnClose);
        // Every action the dialog offers is wired once, here: the window owns the
        // download, the dialog only reports what the user pressed.
        connect(m_updateDialog, &UpdateDialog::downloadRequested, this, [this] {
            m_updateChecker->downloadUpdate();
        });
        connect(m_updateDialog, &UpdateDialog::openPageRequested, this, [](const QUrl& url) {
            if (url.isValid()) {
                QDesktopServices::openUrl(url);
            }
        });
        connect(m_updateDialog, &UpdateDialog::runInstallerRequested, this,
                [](const QString& path) { DownloadManager::openFile(path); });
        connect(m_updateDialog, &UpdateDialog::cancelRequested, m_updateChecker,
                &UpdateChecker::cancelDownload);
    }
    m_updateDialog->offerRelease(info);
    m_updateDialog->show();
    m_updateDialog->raise();
    m_updateDialog->activateWindow();
}

void BrowserWindow::showUpdateProblem(const QString& reason)
{
    if (!m_updateDialog) {
        m_updateDialog = new UpdateDialog(this);
        m_updateDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_updateDialog, &UpdateDialog::openPageRequested, this, [](const QUrl& url) {
            if (url.isValid()) {
                QDesktopServices::openUrl(url);
            }
        });
        connect(m_updateDialog, &UpdateDialog::runInstallerRequested, this,
                [](const QString& path) { DownloadManager::openFile(path); });
    }
    m_updateDialog->showProblem(reason);
    m_updateDialog->show();
    m_updateDialog->raise();
    m_updateDialog->activateWindow();
}

void BrowserWindow::showStatusMessage(const QString& message)
{
    m_navBar->showMessage(message);
}

void BrowserWindow::buildMenu(const QPoint& globalPos)
{
    QMenu menu(this);

    QAction* newTabAction = menu.addAction(tr("New tab\tCtrl+T"));
    connect(newTabAction, &QAction::triggered, this, [this] { newTab(); });

    QAction* newWindowAction = menu.addAction(tr("New window\tCtrl+N"));
    connect(newWindowAction, &QAction::triggered, this, &BrowserWindow::openNewWindow);

    QAction* privateWindowAction = menu.addAction(tr("New private window\tCtrl+Shift+N"));
    connect(privateWindowAction, &QAction::triggered, this, &BrowserWindow::openPrivateWindow);
    menu.addSeparator();

    QAction* bookmarkAction = menu.addAction(tr("Bookmark this page\tCtrl+D"));
    connect(bookmarkAction, &QAction::triggered, this, &BrowserWindow::toggleBookmark);

    QAction* bookmarksAction = menu.addAction(tr("Bookmarks\tCtrl+Shift+O"));
    connect(bookmarksAction, &QAction::triggered, this, [this] { showLibrary(true); });

    QAction* historyAction = menu.addAction(tr("History\tCtrl+H"));
    connect(historyAction, &QAction::triggered, this, [this] { showLibrary(false); });
    menu.addSeparator();

    QAction* settingsAction = menu.addAction(tr("Settings...\tCtrl+,"));
    connect(settingsAction, &QAction::triggered, this, &BrowserWindow::showSettings);

    QAction* clearDataAction = menu.addAction(tr("Clear browsing data..."));
    connect(clearDataAction, &QAction::triggered, this, &BrowserWindow::showClearBrowsingData);
    menu.addSeparator();

    QAction* devToolsAction = menu.addAction(tr("Developer tools\tF12"));
    connect(devToolsAction, &QAction::triggered, this, &BrowserWindow::openDevTools);

    QAction* checkUpdate = menu.addAction(tr("Check for updates"));
    connect(checkUpdate, &QAction::triggered, this, [this] {
        showStatusMessage(tr("Checking for updates..."));
        m_updateChecker->checkNow();
    });
    menu.addSeparator();

    QAction* closeAction = menu.addAction(tr("Close window\tCtrl+Q"));
    connect(closeAction, &QAction::triggered, this, &BrowserWindow::close);

    menu.exec(globalPos);
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void BrowserWindow::updateTabLabel(int)
{
    refreshTabStrip();
}

void BrowserWindow::updateForActiveTab()
{
    auto* tab = currentTab();
    if (!tab) {
        return;
    }
    m_navBar->setCanGoBack(tab->canGoBack());
    m_navBar->setCanGoForward(tab->canGoForward());
    m_navBar->setLoading(tab->isLoading());
    m_navBar->addressBar()->displayUrl(tab->url());
    m_sideBar->setHomeActive(tab->isStartPage());
    updateBookmarkStar();
    refreshTabStrip();
}

void BrowserWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    // The native window handle only exists once the window is shown, so the
    // Mica / Acrylic request has to be made here rather than in the
    // constructor.
    Glass::applyWindowBackdrop(this);
}

void BrowserWindow::closeEvent(QCloseEvent* event)
{
    m_closing = true;
    if (!m_private) {
        m_settings->setWindowGeometry(saveGeometry());
    }
    for (auto& devTools : m_devToolsWindows) {
        if (devTools) {
            devTools->close();
        }
    }
    m_devToolsWindows.clear();
    QMainWindow::closeEvent(event);
}

}  // namespace yozora
