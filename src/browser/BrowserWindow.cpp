// SPDX-License-Identifier: MIT
#include "browser/BrowserWindow.h"

#include "app/AppPaths.h"
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
#include "privacy/RequestInterceptor.h"
#include "ui/AddressBar.h"
#include "ui/ClearBrowsingDataDialog.h"
#include "ui/DownloadsDialog.h"
#include "ui/Icons.h"
#include "ui/LibraryDialog.h"
#include "ui/NavigationBar.h"
#include "ui/SettingsDialog.h"
#include "ui/ShieldDialog.h"
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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScreen>
#include <QShortcut>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWindow>

#if defined(Q_OS_WIN)
#  include <windows.h>
#  include <windowsx.h>
#endif

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
    // Chrome-style frame: no native title bar. The tab strip row carries the
    // window controls, and WM_NCHITTEST (below) restores resizing and snapping.
    setWindowFlag(Qt::FramelessWindowHint, true);

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
    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // The tab strip and window controls span the full width at the top. The
    // sidebar starts below this row, so it never runs the whole height of the
    // window (it used to sit beside the tab strip and reach the very top).
    m_topBar = new QWidget(central);
    m_topBar->setObjectName(QStringLiteral("topBar"));
    // A plain QWidget needs this to paint its style-sheet background, which is
    // the glass gradient the tab strip sits on.
    m_topBar->setAttribute(Qt::WA_StyledBackground, true);
    auto* topLayout = new QHBoxLayout(m_topBar);
    topLayout->setContentsMargins(0, 0, 0, 0);
    topLayout->setSpacing(0);

    m_tabStrip = new TabStrip(m_topBar);

    // One toggle for the sidebar, always in the same place: at the far left of
    // the top bar. It flips between "hide" and "show" instead of the two
    // separate buttons there used to be.
    m_sideBarToggle = new QToolButton(m_topBar);
    m_sideBarToggle->setObjectName(QStringLiteral("sideBarToggle"));
    m_sideBarToggle->setCursor(Qt::PointingHandCursor);
    m_sideBarToggle->setIconSize(QSize(16, 16));
    m_sideBarToggle->setFixedSize(36, 30);
    connect(m_sideBarToggle, &QToolButton::clicked, this, [this] {
        if (m_sideBarEnabled) {
            m_sideBar->setCollapsed(!m_sideBar->isCollapsed());
        }
    });
    topLayout->addWidget(m_sideBarToggle);
    topLayout->addWidget(m_tabStrip, 1);

    auto* controls = new QWidget(m_topBar);
    controls->setObjectName(QStringLiteral("windowControls"));
    auto* controlsLayout = new QHBoxLayout(controls);
    controlsLayout->setContentsMargins(4, 0, 6, 0);
    controlsLayout->setSpacing(2);
    m_minButton = makeWindowButton(icons::Shape::Minimize, QStringLiteral("windowMinButton"),
                                   tr("Minimize"));
    m_maxButton = makeWindowButton(icons::Shape::Maximize, QStringLiteral("windowMaxButton"),
                                   tr("Maximize"));
    m_closeButton = makeWindowButton(icons::Shape::Close, QStringLiteral("windowCloseButton"),
                                     tr("Close"));
    controlsLayout->addWidget(m_minButton);
    controlsLayout->addWidget(m_maxButton);
    controlsLayout->addWidget(m_closeButton);
    topLayout->addWidget(controls);

    connect(m_minButton, &QToolButton::clicked, this, &QWidget::showMinimized);
    connect(m_maxButton, &QToolButton::clicked, this, [this] {
        // isFullScreen() too: after a video's full screen the window is in that
        // state, and the button has to bring it back to a window as well.
        if (isMaximized() || isFullScreen()) {
            showNormal();
        } else {
            showMaximized();
        }
    });
    connect(m_closeButton, &QToolButton::clicked, this, &QWidget::close);

    root->addWidget(m_topBar);

    // The address bar also spans the full width, below the tab row; the rail
    // starts below it.
    m_navBar = new NavigationBar(central);
    m_navBar->setObjectName(QStringLiteral("navigationBar"));
    root->addWidget(m_navBar);

    // The page area. The rail is an overlay on top of it, not part of the
    // layout, so showing or hiding the rail never resizes the page.
    m_contentArea = new QWidget(central);
    auto* contentLayout = new QVBoxLayout(m_contentArea);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    m_pages = new QStackedWidget(m_contentArea);
    m_downloads = new DownloadManager(m_profile->profile(), m_settings, m_contentArea);
    contentLayout->addWidget(m_pages, 1);
    contentLayout->addWidget(m_downloads->statusBar());
    root->addWidget(m_contentArea, 1);

    m_sideBar = new SideBar(m_contentArea);
    m_sideBar->raise();

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
    connect(m_sideBar, &SideBar::downloadsRequested, this,
            &BrowserWindow::showDownloadsManager);
    connect(m_sideBar, &SideBar::privateRequested, this, &BrowserWindow::openPrivateWindow);
    connect(m_sideBar, &SideBar::settingsRequested, this, &BrowserWindow::showSettings);
    // A rail the user hid stays hidden, and a rail they brought back stays
    // visible, for the rest of the session and the next start.
    connect(m_sideBar, &SideBar::collapsedChanged, this, [this](bool collapsed) {
        m_settings->setSideBarCollapsed(collapsed);
        updateSideBarToggle();
        update();
    });
    m_sideBarEnabled = m_settings->sideBarEnabled();
    if (m_settings->sideBarCollapsed()) {
        m_sideBar->setCollapsed(true, false);
    }
    applySideBarEnabled();
    connect(m_settings, &Settings::sideBarEnabledChanged, this,
            &BrowserWindow::applySideBarEnabled);

    // The update signals are wired once, here, rather than inside buildMenu().
    // The menu is rebuilt every time it is opened, and connecting there added one
    // more connection per opening - after a dozen openings a single check would
    // report its result a dozen times.
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
            [this](const QString& reason, ReleaseError) {
                // A failure is worth a dialog: the usual causes (no network, no
                // public release) need a sentence, not a line in a status strip
                // that is gone before it has been read. An automatic check stays
                // silent instead of interrupting whatever the user is doing.
                if (m_silentUpdateCheck) {
                    m_silentUpdateCheck = false;
                    return;
                }
                showUpdateProblem(reason);
            });
    connect(m_updateChecker, &UpdateChecker::updateCheckFinished, this, [this](bool hasUpdate) {
        if (!hasUpdate && !m_silentUpdateCheck) {
            showStatusMessage(tr("Yozora is up to date"));
        }
        m_silentUpdateCheck = false;
    });
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [this](const ReleaseInfo& info) {
                m_silentUpdateCheck = false;
                showUpdateOffer(info);
            });
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
    connect(m_tabStrip, &TabStrip::contextMenuRequested, this,
            &BrowserWindow::showTabContextMenu);

    connect(m_navBar, &NavigationBar::backRequested, this, &BrowserWindow::goBack);
    connect(m_navBar, &NavigationBar::forwardRequested, this, &BrowserWindow::goForward);
    connect(m_navBar, &NavigationBar::reloadRequested, this, &BrowserWindow::reload);
    connect(m_navBar, &NavigationBar::stopRequested, this, &BrowserWindow::stop);
    connect(m_navBar, &NavigationBar::menuRequested, this, &BrowserWindow::buildMenu);
    connect(m_navBar, &NavigationBar::bookmarkRequested, this, &BrowserWindow::toggleBookmark);
    connect(m_navBar, &NavigationBar::shieldRequested, this, &BrowserWindow::showShield);
    connect(m_navBar->addressBar(), &AddressBar::navigationRequested, this,
            &BrowserWindow::navigateInput);

    // Per-page blocking counts for the shield. The interceptor runs on the IO
    // thread and its signal is queued here; only plain values are carried.
    if (m_profile && m_profile->interceptor()) {
        connect(m_profile->interceptor(), &RequestInterceptor::requestBlocked, this,
                [this](const QString& pageUrl, int category) {
                    m_blockingStats.record(pageUrl,
                                           static_cast<FilterEngine::Category>(category));
                    updateShieldState();
                });
    }

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

    // Automatic update checks, off unless the user enabled them. The first runs
    // shortly after start (so it does not compete with loading the first page),
    // then every six hours.
    m_updateTimer = new QTimer(this);
    m_updateTimer->setInterval(6 * 60 * 60 * 1000);
    connect(m_updateTimer, &QTimer::timeout, this, [this] {
        m_silentUpdateCheck = true;
        m_updateChecker->checkNow();
    });
    const auto armUpdateTimer = [this] {
        if (m_settings->backgroundUpdates()) {
            m_updateTimer->start();
            QTimer::singleShot(30000, this, [this] {
                if (m_settings->backgroundUpdates()) {
                    m_silentUpdateCheck = true;
                    m_updateChecker->checkNow();
                }
            });
        } else {
            m_updateTimer->stop();
        }
    };
    armUpdateTimer();
    connect(m_settings, &Settings::backgroundUpdatesChanged, this, armUpdateTimer);
}

QToolButton* BrowserWindow::makeWindowButton(icons::Shape shape, const QString& objectName,
                                             const QString& tooltip)
{
    auto* button = new QToolButton(m_topBar);
    button->setObjectName(objectName);
    button->setToolTip(tooltip);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::ArrowCursor);
    button->setIcon(icons::icon(shape, 18, QColor(Theme::colors().text), 1.35));
    button->setIconSize(QSize(16, 16));
    button->setFixedSize(40, 30);
    return button;
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
        // Leaving a video's full screen is the first thing Esc should do; the
        // page is told too, so its own full-screen state does not get stuck.
        if (m_browserFullScreen || isFullScreen()) {
            if (auto* tab = currentTab()) {
                if (auto* view = tab->view()) {
                    view->page()->runJavaScript(
                        QStringLiteral("document.fullscreenElement && document.exitFullscreen()"));
                }
            }
            setBrowserFullScreen(false);
            return;
        }
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
    add(sequence("Ctrl+B"), [this] {
        if (m_sideBarEnabled) {
            m_sideBar->setCollapsed(!m_sideBar->isCollapsed());
        }
    });
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
    // A plain browser full screen, independent of a page's video full screen.
    add(sequence("F11"), [this] { setBrowserFullScreen(!m_browserFullScreen); });
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

void BrowserWindow::duplicateTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    BrowserTab* source = m_tabs.at(index);
    const QUrl url = source->url();
    // The duplicate opens next to its source, not at the end, and is inserted
    // just after it so the two stay together.
    BrowserTab* tab = new BrowserTab(m_profile->profile(), m_settings, m_history, this);
    connectTab(tab);
    m_pages->addWidget(tab);
    const int at = qMin(index + 1, static_cast<int>(m_tabs.size()));
    m_tabs.insert(at, tab);
    if (source->isStartPage()) {
        tab->showStartPage();
    } else if (url.isValid()) {
        tab->loadUrl(url);
    }
    refreshTabStrip();
    selectTab(at);
}

void BrowserWindow::togglePinTab(int index)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    BrowserTab* tab = m_tabs.at(index);
    const bool wasPinned = m_pinnedTabs.contains(tab);
    if (wasPinned) {
        m_pinnedTabs.remove(tab);
    } else {
        m_pinnedTabs.insert(tab);
    }

    // Pinned tabs sit at the front, unpinned after them; the relative order
    // within each group is preserved. Rebuilt as a stable partition.
    QList<BrowserTab*> pinned;
    QList<BrowserTab*> rest;
    for (BrowserTab* t : m_tabs) {
        if (m_pinnedTabs.contains(t)) {
            pinned.append(t);
        } else {
            rest.append(t);
        }
    }
    QList<BrowserTab*> ordered = pinned;
    ordered += rest;
    const int newIndex = ordered.indexOf(tab);
    m_tabs = ordered;

    refreshTabStrip();
    selectTab(newIndex);
}

void BrowserWindow::showTabContextMenu(int index, const QPoint& globalPos)
{
    if (index < 0 || index >= m_tabs.size()) {
        return;
    }
    BrowserTab* tab = m_tabs.at(index);
    const bool pinned = m_pinnedTabs.contains(tab);

    QMenu menu(this);
    QAction* duplicate = menu.addAction(tr("Duplicate tab"));
    QAction* pin = menu.addAction(pinned ? tr("Unpin tab") : tr("Pin tab"));
    menu.addSeparator();
    QAction* close = menu.addAction(tr("Close tab\tCtrl+W"));
    close->setEnabled(!pinned);

    QAction* chosen = menu.exec(globalPos);
    if (chosen == duplicate) {
        duplicateTab(index);
    } else if (chosen == pin) {
        togglePinTab(index);
    } else if (chosen == close) {
        closeTab(index);
    }
}

// ---------------------------------------------------------------------------
// Session
// ---------------------------------------------------------------------------

void BrowserWindow::scheduleSessionSave()
{
    if (m_private) {
        return;
    }
    if (!m_sessionTimer) {
        m_sessionTimer = new QTimer(this);
        m_sessionTimer->setSingleShot(true);
        connect(m_sessionTimer, &QTimer::timeout, this, [this] { writeSession(false); });
    }
    // Coalesce the flurry of changes that come with opening or closing tabs.
    m_sessionTimer->start(1000);
}

void BrowserWindow::writeSession(bool clean)
{
    if (m_private) {
        return;
    }
    AppPaths::ensureCreated();

    const SessionSnapshot snap = snapshot();
    QJsonObject root;
    root.insert(QStringLiteral("clean"), clean);
    root.insert(QStringLiteral("active"), snap.activeIndex);
    QJsonArray urls;
    for (const QString& url : snap.urls) {
        urls.append(url);
    }
    root.insert(QStringLiteral("urls"), urls);

    QSaveFile file(AppPaths::sessionPath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Compact));
        file.commit();
    }
}

bool BrowserWindow::sessionCrashed() const
{
    QFile file(AppPaths::sessionPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }
    return !document.object().value(QStringLiteral("clean")).toBool(false);
}

bool BrowserWindow::restoreSession()
{
    QFile file(AppPaths::sessionPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        return false;
    }
    const QJsonObject root = document.object();
    const QJsonArray array = root.value(QStringLiteral("urls")).toArray();
    if (array.isEmpty()) {
        return false;
    }

    SessionSnapshot snap;
    for (const QJsonValue& value : array) {
        const QString url = value.toString();
        // Skip blank pages saved by an older build: reopening them gives empty
        // tabs with no purpose.
        if (!url.isEmpty() && !url.startsWith(QLatin1String("about:blank"))) {
            snap.urls.append(url);
        }
    }
    if (snap.urls.isEmpty()) {
        return false;
    }
    snap.activeIndex = root.value(QStringLiteral("active")).toInt();
    restoreSnapshot(snap);
    return true;
}

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
            // The page (a video's full-screen button, typically) asks to go
            // full screen; the window hides its chrome and takes over the whole
            // screen.
            connect(page, &WebPage::fullScreenRequested, this,
                    &BrowserWindow::setBrowserFullScreen);
        }
        connect(view, &WebView::zoomChanged, this, [this, tab](int percent) {
            if (tab == currentTab()) {
                m_navBar->showMessage(tr("Zoom %1%").arg(percent));
            }
        });
        connect(view, &WebView::fileDropped, this,
                [tab](const QUrl& url) { tab->openLocalFile(url); });
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
        entry.pinned = m_pinnedTabs.contains(tab);
        entry.id = reinterpret_cast<quintptr>(tab);
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
    m_pinnedTabs.remove(tab);
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
        for (const QString& url : tab->openUrls()) {
            // A tab that ended up at about:blank (Chromium parks a blocked
            // navigation there) is empty and not worth reopening; saving it was
            // why a fresh start could come back with blank tabs.
            if (url.isEmpty() || url.startsWith(QLatin1String("about:blank"))) {
                continue;
            }
            snap.urls += url;
        }
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
    m_pinnedTabs.clear();

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

void BrowserWindow::showDownloadsManager()
{
    DownloadsDialog dialog(m_downloads, this);
    dialog.exec();
}

void BrowserWindow::setBrowserFullScreen(bool fullScreen)
{
    if (m_browserFullScreen == fullScreen) {
        return;
    }
    m_browserFullScreen = fullScreen;

    if (fullScreen) {
        // Remember whether the window was maximized so leaving full screen puts
        // it back the way it was.
        m_wasMaximizedBeforeFullScreen = isMaximized();
        // Hide the browser chrome: tab strip, navigation bar and the download
        // strip. On a video this is what makes it cover the whole screen.
        m_tabStrip->setVisible(false);
        m_navBar->setVisible(false);
        if (m_sideBar) {
            m_sideBar->setVisible(false);
        }
        if (m_sideBarToggle) {
            m_sideBarToggle->setVisible(false);
        }
        if (m_downloads && m_downloads->statusBar()) {
            m_downloads->statusBar()->setVisible(false);
        }
        showFullScreen();
    } else {
        m_tabStrip->setVisible(true);
        m_navBar->setVisible(true);
        // Re-applies the rail's own enabled/visible state.
        applySideBarEnabled();
        if (m_downloads && m_downloads->statusBar()) {
            m_downloads->statusBar()->setVisible(m_downloads->activeDownloadCount() > 0);
        }
        // showMaximized()/showNormal() rather than setWindowState(): going back
        // through setWindowState made Windows animate a minimize/restore flash
        // on the way out.
        if (m_wasMaximizedBeforeFullScreen) {
            showMaximized();
        } else {
            showNormal();
        }
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

    QAction* shieldAction = menu.addAction(tr("Ads and trackers blocked..."));
    connect(shieldAction, &QAction::triggered, this, &BrowserWindow::showShield);

    QAction* bookmarksAction = menu.addAction(tr("Bookmarks\tCtrl+Shift+O"));
    connect(bookmarksAction, &QAction::triggered, this, [this] { showLibrary(true); });

    QAction* historyAction = menu.addAction(tr("History\tCtrl+H"));
    connect(historyAction, &QAction::triggered, this, [this] { showLibrary(false); });

    QAction* downloadsAction = menu.addAction(tr("Downloads"));
    connect(downloadsAction, &QAction::triggered, this, &BrowserWindow::showDownloadsManager);
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
    updateShieldState();
    refreshTabStrip();
    scheduleSessionSave();
}

void BrowserWindow::updateShieldState()
{
    if (!m_navBar) {
        return;
    }
    auto* tab = currentTab();
    const QUrl url = tab ? tab->url() : QUrl();
    const QString pageUrl = url.toString();
    const int blocked = pageUrl.isEmpty() ? 0 : m_blockingStats.blockedForPage(pageUrl);
    const bool enabled = m_settings ? m_settings->blockAds() : true;
    m_navBar->setShieldState(blocked, enabled);
}

void BrowserWindow::showShield()
{
    auto* tab = currentTab();
    const QUrl url = tab ? tab->url() : QUrl();
    const QString host = url.host().toLower();
    const QString pageUrl = url.toString();
    const int blocked = pageUrl.isEmpty() ? 0 : m_blockingStats.blockedForPage(pageUrl);
    const bool enabled = m_settings ? m_settings->blockAds() : true;
    const bool allowed = m_settings && !host.isEmpty()
                         && m_settings->isSiteAllowedForAdBlock(host);

    if (!m_shieldDialog) {
        m_shieldDialog = new ShieldDialog(this);
        m_shieldDialog->setAttribute(Qt::WA_DeleteOnClose);
        connect(m_shieldDialog, &ShieldDialog::siteAllowedChanged, this,
                [this](bool allow) {
                    auto* current = currentTab();
                    const QString currentHost =
                        current ? current->url().host().toLower() : QString();
                    if (currentHost.isEmpty() || !m_settings) {
                        return;
                    }
                    if (allow) {
                        m_settings->allowSiteForAdBlock(currentHost);
                    } else {
                        m_settings->disallowSiteForAdBlock(currentHost);
                    }
                    updateShieldState();
                    // A reload is the only way to bring blocked requests back
                    // once the site is allowed (or drop them once blocked).
                    if (current) {
                        current->reload();
                    }
                });
    }
    m_shieldDialog->setSite(host, blocked, enabled, allowed);
    m_shieldDialog->show();
    m_shieldDialog->raise();
    m_shieldDialog->activateWindow();
}

void BrowserWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    layoutOverlays();
}

void BrowserWindow::layoutOverlays()
{
    if (!m_contentArea || !m_sideBar) {
        return;
    }
    // The rail floats over the page area; the parent only supplies the height.
    m_sideBar->setAvailableHeight(m_contentArea->height());
}

void BrowserWindow::updateSideBarToggle()
{
    if (!m_sideBarToggle) {
        return;
    }
    const bool collapsed = m_sideBar->isCollapsed();
    m_sideBarToggle->setIcon(icons::icon(collapsed ? icons::Shape::ChevronRight
                                                   : icons::Shape::ChevronLeft,
                                         18, QColor(Theme::colors().textMuted), 1.3));
    m_sideBarToggle->setToolTip(collapsed ? tr("Show the sidebar (Ctrl+B)")
                                          : tr("Hide the sidebar (Ctrl+B)"));
}

void BrowserWindow::applySideBarEnabled()
{
    m_sideBarEnabled = m_settings->sideBarEnabled();
    m_sideBar->setVisible(m_sideBarEnabled);
    m_sideBarToggle->setVisible(m_sideBarEnabled);
    if (m_sideBarEnabled) {
        // Snap the rail to its resting place for the current state, then refresh
        // the toggle glyph.
        m_sideBar->setCollapsed(m_sideBar->isCollapsed(), false);
        layoutOverlays();
        updateSideBarToggle();
    }
}

void BrowserWindow::showEvent(QShowEvent* event)
{
    QMainWindow::showEvent(event);
    // The native window handle only exists once the window is shown, so the
    // Mica / Acrylic request has to be made here rather than in the
    // constructor.
    Glass::applyWindowBackdrop(this);
}

void BrowserWindow::changeEvent(QEvent* event)
{
    QMainWindow::changeEvent(event);
    if (event->type() == QEvent::WindowStateChange && m_maxButton) {
        const bool max = isMaximized();
        m_maxButton->setIcon(icons::icon(max ? icons::Shape::Restore : icons::Shape::Maximize, 18,
                                         QColor(Theme::colors().text)));
        m_maxButton->setToolTip(max ? tr("Restore") : tr("Maximize"));
    }
}

#if defined(Q_OS_WIN)
bool BrowserWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
    if (eventType == "windows_generic_MSG" || eventType == "windows_dispatcher_MSG") {
        MSG* msg = static_cast<MSG*>(message);
        if (msg->message == WM_NCHITTEST) {
            // lParam is in physical screen pixels; convert to Qt's logical
            // coordinates so the test is right on scaled displays too.
            qreal dpr = 1.0;
            if (windowHandle() && windowHandle()->screen()) {
                dpr = windowHandle()->screen()->devicePixelRatio();
            }
            const QPoint global(qRound(GET_X_LPARAM(msg->lParam) / dpr),
                                qRound(GET_Y_LPARAM(msg->lParam) / dpr));
            const QPoint local = mapFromGlobal(global);
            const QRect r = rect();

            // Eight pixels of grab area on each edge, but only when the window is
            // not maximized (there is nothing to resize then).
            constexpr int kBorder = 8;
            if (!isMaximized()) {
                const bool left = local.x() < kBorder;
                const bool right = local.x() >= r.width() - kBorder;
                const bool top = local.y() < kBorder;
                const bool bottom = local.y() >= r.height() - kBorder;
                if (top && left) { *result = HTTOPLEFT; return true; }
                if (top && right) { *result = HTTOPRIGHT; return true; }
                if (bottom && left) { *result = HTBOTTOMLEFT; return true; }
                if (bottom && right) { *result = HTBOTTOMRIGHT; return true; }
                if (left) { *result = HTLEFT; return true; }
                if (right) { *result = HTRIGHT; return true; }
                if (top) { *result = HTTOP; return true; }
                if (bottom) { *result = HTBOTTOM; return true; }
            }

            // The empty parts of the tab strip drag the window; returning
            // HTCAPTION also gives Windows' snap and double-click-to-maximize.
            // Tabs, the "+" and the window controls are not drag regions, so
            // they keep receiving clicks.
            if (m_tabStrip && m_tabStrip->isVisible()) {
                const QPoint stripLocal = m_tabStrip->mapFrom(this, local);
                if (m_tabStrip->rect().contains(stripLocal)
                    && m_tabStrip->isDragRegion(stripLocal)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
        }
    }
    return QMainWindow::nativeEvent(eventType, message, result);
}
#endif

void BrowserWindow::closeEvent(QCloseEvent* event)
{
    m_closing = true;
    if (!m_private) {
        m_settings->setWindowGeometry(saveGeometry());
        // A clean exit is what tells the next start that the saved tabs are a
        // normal session and not the remains of a crash.
        writeSession(true);
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
