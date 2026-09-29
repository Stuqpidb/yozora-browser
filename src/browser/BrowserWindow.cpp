// SPDX-License-Identifier: MIT
#include "browser/BrowserWindow.h"

#include "browser/BrowserTab.h"
#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "core/Theme.h"
#include "core/UpdateChecker.h"
#include "ui/AddressBar.h"
#include "ui/NavigationBar.h"
#include "ui/SettingsDialog.h"
#include "ui/TabWidget.h"
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
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QShortcut>
#include <QSplitter>
#include <QStatusBar>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWebEngineView>

namespace yozora {

namespace {
constexpr int kMaxClosedTabs = 12;
}

BrowserWindow::BrowserWindow(WebProfile* profile, Settings* settings, QWidget* parent)
    : QMainWindow(parent)
    , m_profile(profile)
    , m_settings(settings)
    , m_updateChecker(new UpdateChecker(this))
{
    buildUi();
    buildShortcuts();

    connect(m_settings, &Settings::themeModeChanged, this, [this] { applyTheme(isDark()); });
    connect(m_settings, &Settings::searchEngineChanged, this, [this] {
        if (auto* tab = currentTab()) {
            tab->updateSearchEngineUi();
        }
    });

    applyTheme(isDark());
    newTab();

    const QByteArray geometry = m_settings->windowGeometry();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    } else {
        resize(1280, 820);
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
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_navBar = new NavigationBar(central);
    m_navBar->setObjectName(QStringLiteral("navigationBar"));
    m_navBar->setBrand(QStringLiteral("Yozora"), QString::fromLatin1(kVersionString));

    m_tabWidget = new TabWidget(central);
    m_tabWidget->setObjectName(QStringLiteral("tabWidget"));

    m_downloads = new DownloadManager(m_profile->profile(), m_settings, central);

    layout->addWidget(m_navBar);
    layout->addWidget(m_tabWidget, 1);
    layout->addWidget(m_downloads->statusBar());

    setCentralWidget(central);
    statusBar()->hide();

    connect(m_navBar, &NavigationBar::backRequested, this, &BrowserWindow::goBack);
    connect(m_navBar, &NavigationBar::forwardRequested, this, &BrowserWindow::goForward);
    connect(m_navBar, &NavigationBar::reloadRequested, this, &BrowserWindow::reload);
    connect(m_navBar, &NavigationBar::stopRequested, this, &BrowserWindow::stop);
    connect(m_navBar, &NavigationBar::menuRequested, this, &BrowserWindow::buildMenu);

    connect(m_navBar->addressBar(), &AddressBar::navigationRequested, this,
            &BrowserWindow::navigateInput);

    connect(m_tabWidget, &TabWidget::newTabRequested, this, [this] { newTab(); });
    connect(m_tabWidget, &TabWidget::closeRequested, this, &BrowserWindow::closeTab);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, [this](int) { updateForActiveTab(); });

    connect(m_downloads, &DownloadManager::downloadStarted, this,
            [this](const QString& name) { showStatusMessage(tr("Downloading %1").arg(name)); });
    connect(m_downloads, &DownloadManager::downloadFinished, this, [this](const QString& path) {
        showStatusMessage(tr("Downloaded %1")
                              .arg(QFileInfo(path).fileName()));
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

    // Key sequences are written as text so the map reads the same on every
    // platform and can be copied straight into the documentation.
    const auto sequence = [](const char* text) {
        return QKeySequence(QString::fromLatin1(text));
    };

    // --- tabs and windows ---
    add(sequence("Ctrl+T"), [this] { newTab(); });
    add(sequence("Ctrl+W"), [this] { closeCurrentTab(); });
    add(sequence("Ctrl+Shift+T"), [this] { restoreLastClosedTab(); });
    add(sequence("Ctrl+N"), [this] { openNewWindow(); });

    // Ctrl+1..9 jump to a tab, Ctrl+9 also reaches the last one.
    for (int i = 1; i <= 9; ++i) {
        add(sequence(QStringLiteral("Ctrl+%1").arg(i).toLatin1().constData()),
            [this, i] {
                const int index = i - 1;
                if (index < m_tabWidget->count()) {
                    m_tabWidget->setCurrentIndex(index);
                } else if (i == 9) {
                    m_tabWidget->setCurrentIndex(m_tabWidget->count() - 1);
                }
            });
    }

    // --- navigation ---
    add(sequence("Alt+Left"), [this] { goBack(); });
    add(sequence("Alt+Right"), [this] { goForward(); });
    add(sequence("Ctrl+R"), [this] { reload(); });
    add(sequence("F5"), [this] { reload(); });
    add(sequence("Esc"), [this] {
        if (m_navBar->addressBar()->hasFocus()) {
            m_navBar->addressBar()->clear();
            if (QWidget* page = m_tabWidget->currentWidget()) {
                page->setFocus();
            }
        } else {
            stop();
        }
    });

    // --- address bar ---
    add(sequence("Ctrl+L"), [this] { focusAddressBar(); });

    // --- window ---
    add(sequence("Ctrl+Shift+I"), [this] { openDevTools(); });
    add(sequence("F12"), [this] { openDevTools(); });
    add(sequence("Ctrl+,"), [this] { showSettings(); });
    add(sequence("Ctrl+Q"), [this] { close(); });

    // --- zoom ---
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
    auto* tab = new BrowserTab(m_profile->profile(), m_settings, this);
    const int index = m_tabWidget->appendTab(tab, TabWidget::defaultTitle());
    tab->setDarkMode(isDark());
    connectTab(tab);

    if (url.isValid() && !url.isEmpty()) {
        tab->loadUrl(url);
    } else {
        tab->showStartPage();
    }

    if (foreground) {
        m_tabWidget->setCurrentIndex(index);
        m_navBar->addressBar()->setFocus();
    }

    updateTabLabel(index);
    if (m_tabWidget->currentIndex() == index) {
        updateForActiveTab();
    }
    return tab;
}

int BrowserWindow::tabCount() const
{
    return m_tabWidget->count();
}

void BrowserWindow::openInFirstTab(const QUrl& url)
{
    if (m_tabWidget->count() != 1) {
        newTab(url);
        return;
    }
    if (auto* tab = currentTab()) {
        tab->loadUrl(url);
    }
}

BrowserTab* BrowserWindow::currentTab() const
{
    return qobject_cast<BrowserTab*>(m_tabWidget->currentWidget());
}

void BrowserWindow::connectTab(BrowserTab* tab)
{
    connect(tab, &BrowserTab::urlChanged, this, &BrowserWindow::updateForActiveTab);
    connect(tab, &BrowserTab::titleChanged, this, [this, tab] {
        const int index = m_tabWidget->indexOf(tab);
        if (index >= 0) {
            updateTabLabel(index);
            if (index == m_tabWidget->currentIndex()) {
                updateForActiveTab();
            }
        }
    });

    // The navigation bar follows the active tab, not the tab that happens to
    // be reporting, so every state change is filtered by the current index.
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
}

void BrowserWindow::closeTab(int index)
{
    if (index < 0 || index >= m_tabWidget->count()) {
        return;
    }

    m_closedTabs.prepend(snapshot());
    while (m_closedTabs.size() > kMaxClosedTabs) {
        m_closedTabs.removeLast();
    }

    const int next = m_tabWidget->preferredNextTabIndex(index);
    QWidget* removed = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    // Deleting the tab tears down its WebEngine page and renderer resources.
    if (removed) {
        removed->deleteLater();
    }

    if (m_tabWidget->count() == 0) {
        if (m_closing) {
            return;
        }
        close();
        return;
    }

    if (next >= 0) {
        m_tabWidget->setCurrentIndex(next);
    }
    updateForActiveTab();
}

void BrowserWindow::closeCurrentTab()
{
    closeTab(m_tabWidget->currentIndex());
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
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* tab = qobject_cast<BrowserTab*>(m_tabWidget->widget(i))) {
            snap.urls += tab->openUrls();
        }
    }
    snap.activeIndex = m_tabWidget->currentIndex();
    return snap;
}

void BrowserWindow::restoreSnapshot(const SessionSnapshot& snap)
{
    if (snap.urls.isEmpty()) {
        newTab();
        return;
    }

    while (m_tabWidget->count() > 0) {
        QWidget* old = m_tabWidget->widget(0);
        m_tabWidget->removeTab(0);
        if (old) {
            old->deleteLater();
        }
    }

    int active = 0;
    for (int i = 0; i < snap.urls.size(); ++i) {
        const QUrl url(snap.urls.at(i));
        BrowserTab* tab = newTab(url, false);
        if (i == snap.activeIndex) {
            active = m_tabWidget->indexOf(tab);
        }
    }
    m_tabWidget->setCurrentIndex(qBound(0, active, m_tabWidget->count() - 1));
    updateForActiveTab();
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
    auto* window = new BrowserWindow(m_profile, m_settings);
    window->setAttribute(Qt::WA_DeleteOnClose);
    window->resize(1100, 740);
    window->show();
}

void BrowserWindow::showSettings()
{
    SettingsDialog dialog(m_settings, this);
    dialog.exec();
    if (auto* tab = currentTab()) {
        tab->updateSearchEngineUi();
    }
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

    QAction* newWindowAction = menu.addAction(tr("New window\tCtrl+Shift+N"));
    connect(newWindowAction, &QAction::triggered, this, &BrowserWindow::openNewWindow);

    QAction* settingsAction = menu.addAction(tr("Settings...\tCtrl+,"));
    connect(settingsAction, &QAction::triggered, this, &BrowserWindow::showSettings);
    menu.addSeparator();

    QAction* devToolsAction = menu.addAction(tr("Developer tools\tF12"));
    connect(devToolsAction, &QAction::triggered, this, &BrowserWindow::openDevTools);

    QAction* checkUpdate = menu.addAction(tr("Check for updates"));
    connect(checkUpdate, &QAction::triggered, this, [this] {
        showStatusMessage(tr("Checking for updates..."));
        m_updateChecker->checkNow();
    });
    connect(m_updateChecker, &UpdateChecker::updateAvailable, this,
            [this](const QString& version, const QUrl& url) {
                m_navBar->showMessage(tr("New version available: %1").arg(version), 8000);
                QDesktopServices::openUrl(url);
            });
    connect(m_updateChecker, &UpdateChecker::updateCheckFailed, this,
            [this](const QString& reason) { showStatusMessage(reason); });
    connect(m_updateChecker, &UpdateChecker::updateCheckFinished, this, [this](bool hasUpdate) {
        if (!hasUpdate) {
            showStatusMessage(tr("Yozora is up to date"));
        }
    });

    menu.addSeparator();
    auto* closeAction = menu.addAction(tr("Close window\tCtrl+Q"));
    connect(closeAction, &QAction::triggered, this, &BrowserWindow::close);

    menu.exec(globalPos);
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

void BrowserWindow::updateTabLabel(int index)
{
    auto* tab = qobject_cast<BrowserTab*>(m_tabWidget->widget(index));
    if (!tab) {
        return;
    }
    QString label = tab->title();
    if (label.trimmed().isEmpty()) {
        label = TabWidget::defaultTitle();
    }
    if (label.size() > 48) {
        label = label.left(47) + QChar(0x2026);
    }
    m_tabWidget->updateTab(index, label, tab->icon());
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
    updateTabLabel(m_tabWidget->currentIndex());
}

bool BrowserWindow::isDark() const
{
    return m_settings->themeMode() != Settings::ThemeMode::Light;
}

void BrowserWindow::applyTheme(bool dark)
{
    Theme::apply(dark);
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        if (auto* tab = qobject_cast<BrowserTab*>(m_tabWidget->widget(i))) {
            tab->setDarkMode(dark);
        }
    }
    update();
}

void BrowserWindow::closeEvent(QCloseEvent* event)
{
    m_closing = true;
    m_settings->setWindowGeometry(saveGeometry());
    for (auto& devTools : m_devToolsWindows) {
        if (devTools) {
            devTools->close();
        }
    }
    m_devToolsWindows.clear();
    QMainWindow::closeEvent(event);
}

}  // namespace yozora
