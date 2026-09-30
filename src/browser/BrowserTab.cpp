// SPDX-License-Identifier: MIT
#include "browser/BrowserTab.h"

#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "ui/NewTabPage.h"
#include "utils/UrlUtils.h"
#include "web/ErrorPage.h"
#include "web/WebPage.h"
#include "web/WebView.h"

#include <QStackedWidget>
#include <QVBoxLayout>
#include <QWebEngineHistory>
#include <QWebEngineProfile>

namespace yozora {

QUrl BrowserTab::startPageUrl()
{
    return QUrl(QStringLiteral("about:yozora"));
}

BrowserTab::BrowserTab(QWebEngineProfile* profile, Settings* settings, QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
{
    m_stack = new QStackedWidget(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_startPage = new NewTabPage(m_stack);
    m_view = new WebView(profile, m_stack);
    auto* page = qobject_cast<WebPage*>(m_view->page());
    if (page) {
        page->setDarkMode(m_dark);
    }

    m_stack->addWidget(m_startPage);
    m_stack->addWidget(m_view);
    m_stack->setCurrentIndex(kStartPageIndex);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    if (page) {
        connect(page, &WebPage::titleChanged, this, &BrowserTab::updateTitle);
        connect(page, &WebPage::iconChanged, this, &BrowserTab::updateTitle);
        connect(page, &WebPage::urlChanged, this, &BrowserTab::updateState);
        connect(page, &WebPage::loadingChanged, this, [this](bool loading) {
            emit loadingChanged(loading);
            emit loadProgressChanged(loading ? 0 : 100);
            updateState();
        });
        connect(page, &WebPage::loadProgressChanged, this, &BrowserTab::loadProgressChanged);
        connect(page, &WebPage::newWindowRequested, this, &BrowserTab::newTabRequested);
        connect(page, &WebPage::fullScreenRequested, this, [this](bool fullScreen) {
            // QWidget has no setFullScreen(); the state bit is the supported way.
            Qt::WindowStates state = m_view->windowState();
            state = fullScreen ? (state | Qt::WindowFullScreen)
                               : (state & ~Qt::WindowFullScreen);
            m_view->setWindowState(state);
        });
        connect(m_view, &WebView::newTabRequested, this, &BrowserTab::newTabRequested);
        connect(m_view, &WebView::statusMessage, this, &BrowserTab::statusMessage);
        connect(m_view, &QWebEngineView::loadFinished, this, [this](bool) { updateState(); });
    }

    connect(m_startPage, &NewTabPage::searchRequested, this,
            [this](const QString& query) { loadInput(query, false); });

    if (m_settings) {
        m_view->setScrollMode(m_settings->scrollMode());
        connect(m_settings, &Settings::scrollModeChanged, this,
                [this] { m_view->setScrollMode(m_settings->scrollMode()); });
    }

    updateSearchEngineUi();
    m_lastUrl = startPageUrl();
    m_lastTitle = tr("New Tab");
}

QUrl BrowserTab::url() const
{
    if (isStartPage()) {
        return startPageUrl();
    }
    if (auto* page = qobject_cast<WebPage*>(m_view->page())) {
        // An error page replaces the document, but the user should still see
        // the address that failed in the address bar.
        if (page->showingErrorPage() || page->url().scheme() == QLatin1String("yozora-error")) {
            return page->errorPageFor();
        }
    }
    return m_view->url();
}

QString BrowserTab::title() const
{
    if (isStartPage()) {
        return tr("New Tab");
    }
    if (auto* page = qobject_cast<WebPage*>(m_view->page())) {
        if (page->showingErrorPage() || page->url().scheme() == QLatin1String("yozora-error")) {
            return page->errorTitle();
        }
    }
    const QString pageTitle = m_view->title().trimmed();
    if (!pageTitle.isEmpty()) {
        return pageTitle;
    }
    return url::toDisplayString(m_view->url().toString());
}

QIcon BrowserTab::icon() const
{
    if (isStartPage()) {
        return QIcon();
    }
    return m_view->icon();
}

bool BrowserTab::isStartPage() const
{
    return m_stack->currentIndex() == kStartPageIndex;
}

bool BrowserTab::canGoBack() const
{
    return !isStartPage() && m_view->page()->history()->canGoBack();
}

bool BrowserTab::canGoForward() const
{
    return !isStartPage() && m_view->page()->history()->canGoForward();
}

bool BrowserTab::isLoading() const
{
    if (isStartPage()) {
        return false;
    }
    const auto* page = qobject_cast<WebPage*>(m_view->page());
    return page ? page->isLoading() : false;
}

int BrowserTab::loadProgress() const
{
    const auto* page = qobject_cast<WebPage*>(m_view->page());
    return isStartPage() || !page ? 100 : page->loadProgress();
}

void BrowserTab::showStartPage()
{
    m_stack->setCurrentIndex(kStartPageIndex);
    m_startPage->reset();
    m_lastUrl = startPageUrl();
    m_lastTitle = tr("New Tab");
    emit titleChanged();
    emit urlChanged();
    emit canGoBackChanged(false);
    emit canGoForwardChanged(false);
    updateSearchEngineUi();
    m_startPage->focusSearch();
}

void BrowserTab::showWebPage()
{
    if (m_stack->currentIndex() != kPageIndex) {
        m_stack->setCurrentIndex(kPageIndex);
    }
}

void BrowserTab::loadUrl(const QUrl& target)
{
    if (target.scheme() == QLatin1String("about")
        && target.host() == QLatin1String("yozora")) {
        showStartPage();
        return;
    }

    showWebPage();
    auto* page = qobject_cast<WebPage*>(m_view->page());
    if (page) {
        page->loadUrl(target);
    } else {
        m_view->load(target);
    }
}

void BrowserTab::loadInput(const QString& text, bool isSearch)
{
    if (isSearch) {
        const auto engine =
            m_settings ? m_settings->searchEngine()
                       : SearchEngines::byId(SearchEngines::defaultId());
        const QString query = url::toSearchQuery(text);
        const QString target = engine.urlForQuery(query);
        if (target.isEmpty()) {
            return;
        }
        loadUrl(QUrl(target));
        return;
    }

    const QString normalized = url::normalize(text);
    if (normalized.isEmpty()) {
        // Not a URL after all: treat it as a search so nothing is a dead end.
        loadInput(text, true);
        return;
    }
    loadUrl(QUrl(normalized));
}

void BrowserTab::goBack()
{
    if (!canGoBack()) {
        return;
    }
    m_view->page()->triggerAction(QWebEnginePage::Back);
}

void BrowserTab::goForward()
{
    if (!canGoForward()) {
        return;
    }
    m_view->page()->triggerAction(QWebEnginePage::Forward);
}

void BrowserTab::reload()
{
    if (isStartPage()) {
        m_startPage->reset();
        return;
    }
    m_view->page()->triggerAction(QWebEnginePage::Reload);
}

void BrowserTab::stop()
{
    if (isStartPage()) {
        return;
    }
    m_view->page()->triggerAction(QWebEnginePage::Stop);
}

void BrowserTab::setDarkMode(bool dark)
{
    m_dark = dark;
    m_startPage->setDarkMode(dark);
    m_view->setDarkMode(dark);
    if (auto* page = qobject_cast<WebPage*>(m_view->page())) {
        page->setDarkMode(dark);
    }
}

void BrowserTab::updateSearchEngineUi()
{
    const auto engine =
        m_settings ? m_settings->searchEngine()
                   : SearchEngines::byId(SearchEngines::defaultId());
    m_startPage->setSearchEngineName(engine.name);
    m_startPage->setSearchEnginePlaceholder(tr("Search the web with %1").arg(engine.name));
}

void BrowserTab::updateTitle()
{
    m_lastTitle = title();
    emit titleChanged();
}

void BrowserTab::updateState()
{
    if (isStartPage()) {
        return;
    }
    const QUrl current = m_view->url();
    if (current == m_lastUrl) {
        return;
    }
    m_lastUrl = current;
    emit urlChanged();
    emit canGoBackChanged(canGoBack());
    emit canGoForwardChanged(canGoForward());
    updateTitle();
}

QStringList BrowserTab::openUrls() const
{
    if (isStartPage()) {
        return {QStringLiteral("about:yozora")};
    }
    return {m_view->url().toString()};
}

BrowserTab::~BrowserTab() = default;

}  // namespace yozora
