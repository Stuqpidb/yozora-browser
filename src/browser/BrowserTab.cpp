// SPDX-License-Identifier: MIT
#include "browser/BrowserTab.h"

#include "core/HistoryStore.h"
#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "home/HomePage.h"
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

BrowserTab::BrowserTab(QWebEngineProfile* profile, Settings* settings, HistoryStore* history,
                       QWidget* parent)
    : QWidget(parent)
    , m_settings(settings)
    , m_history(history)
{
    m_stack = new QStackedWidget(this);
    m_stack->setContentsMargins(0, 0, 0, 0);

    m_home = new HomePage(m_stack);

    m_view = new WebView(profile, m_stack);
    auto* page = qobject_cast<WebPage*>(m_view->page());

    m_stack->addWidget(m_home);
    m_stack->addWidget(m_view);
    m_stack->setCurrentIndex(kHomePageIndex);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    connect(m_home, &HomePage::openUrl, this, &BrowserTab::loadUrl);
    connect(m_home, &HomePage::searchRequested, this,
            [this](const QString& text) { loadInput(text, false); });

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
            Qt::WindowStates state = m_view->windowState();
            state = fullScreen ? (state | Qt::WindowFullScreen)
                               : (state & ~Qt::WindowFullScreen);
            m_view->setWindowState(state);
        });
        connect(m_view, &WebView::newTabRequested, this, &BrowserTab::newTabRequested);
        connect(m_view, &WebView::statusMessage, this, &BrowserTab::statusMessage);
        connect(m_view, &QWebEngineView::loadFinished, this, [this](bool ok) {
            updateState();
            if (ok && m_history) {
                const QUrl current = m_view->url();
                if (current.isValid() && !current.isEmpty()) {
                    m_history->record(current.toString(), m_view->title());
                }
            }
        });
    }

    if (m_settings) {
        m_view->setScrollMode(m_settings->scrollMode());
        connect(m_settings, &Settings::scrollModeChanged, this,
                [this] { m_view->setScrollMode(m_settings->scrollMode()); });
    }

    m_lastUrl = startPageUrl();
    m_lastTitle = tr("New Tab");
}

QUrl BrowserTab::url() const
{
    if (isStartPage()) {
        return startPageUrl();
    }
    if (auto* page = qobject_cast<WebPage*>(m_view->page())) {
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
    return m_stack->currentIndex() == kHomePageIndex;
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
    m_stack->setCurrentIndex(kHomePageIndex);
    m_lastUrl = startPageUrl();
    m_lastTitle = tr("New Tab");
    emit titleChanged();
    emit urlChanged();
    emit canGoBackChanged(false);
    emit canGoForwardChanged(false);
    m_home->focusSearch();
}

void BrowserTab::showWebPage()
{
    if (m_stack->currentIndex() != kPageIndex) {
        m_stack->setCurrentIndex(kPageIndex);
    }
}

void BrowserTab::loadUrl(const QUrl& target)
{
    if (target.scheme() == QLatin1String("about") && target.host() == QLatin1String("yozora")) {
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
        const auto engine = m_settings ? m_settings->searchEngine()
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
