// SPDX-License-Identifier: MIT
#include "web/WebPage.h"

#include "web/ErrorPage.h"

#include <QFile>
#include <QLoggingCategory>
#include <QSet>
#include <QWebEngineCertificateError>
#include <QWebEngineFullScreenRequest>
#include <QWebEngineNewWindowRequest>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace yozora {

Q_LOGGING_CATEGORY(lcWebPage, "yozora.web")

WebPage::WebPage(QWebEngineProfile* profile, QObject* parent)
    : QWebEnginePage(profile, parent)
{
    setUrl(QUrl(QStringLiteral("about:blank")));

    connect(this, &QWebEnginePage::loadingChanged, this, &WebPage::onLoadingChanged);
    connect(this, &QWebEnginePage::loadFinished, this, &WebPage::onLoadFinished);
    connect(this, &QWebEnginePage::titleChanged, this, &WebPage::onTitleChanged);
    connect(this, &QWebEnginePage::urlChanged, this, &WebPage::onUrlChanged);
    connect(this, &QWebEnginePage::iconChanged, this, &WebPage::onIconChanged);
    connect(this, &QWebEnginePage::fullScreenRequested, this, &WebPage::onFullScreenRequest);
    connect(this, &QWebEnginePage::newWindowRequested, this, &WebPage::onNewWindowRequested);
    connect(this, &QWebEnginePage::renderProcessTerminated, this,
            &WebPage::onRenderProcessTerminated);
    connect(this, &QWebEnginePage::certificateError, this, &WebPage::onCertificateError);
}

void WebPage::setErrorPageEnabled(bool enabled)
{
    m_errorPageEnabled = enabled;
    settings()->setAttribute(QWebEngineSettings::ErrorPageEnabled, !enabled);
}

void WebPage::setDarkMode(bool dark)
{
    m_dark = dark;
}

bool WebPage::isTransientScheme(const QUrl& url)
{
    const QString scheme = url.scheme();
    return scheme == QLatin1String("about") || scheme == QLatin1String("qrc")
           || scheme == QLatin1String("data") || scheme == QLatin1String("yozora-error");
}

void WebPage::loadUrl(const QUrl& url)
{
    if (!url.isValid()) {
        showErrorPage(url, static_cast<int>(QWebEngineLoadingInfo::ErrorDomain::InternalErrorDomain),
                      -300, QStringLiteral("The address is not valid."));
        return;
    }
    m_requestedUrl = url;
    load(url);
}

void WebPage::showErrorPage(const QUrl& url, int errorDomain, int errorCode,
                            const QString& errorText)
{
    m_showingErrorPage = true;
    m_errorUrl = url;
    m_errorTitle = ErrorPage::titleFor(errorDomain, errorCode);
    setHtml(ErrorPage::html(url, errorDomain, errorCode, errorText, m_dark),
            QUrl(QStringLiteral("yozora-error://error")));
    m_showingErrorPage = false;
}

void WebPage::handleFailure(const QUrl& url, int errorDomain, int errorCode,
                            const QString& errorText)
{
    if (!m_errorPageEnabled || m_showingErrorPage) {
        return;
    }
    // about:blank is the document every page starts with; a "failure" for it is
    // never something the user asked to open. A navigation that turned into a
    // download also ends here, and that must not be reported as a broken page.
    if (url.isEmpty() || url.scheme() == QLatin1String("about")
        || url.scheme() == QLatin1String("yozora-error")
        || url.scheme() == QLatin1String("data")) {
        return;
    }
    showErrorPage(url, errorDomain, errorCode, errorText);
}

void WebPage::onLoadingChanged(const QWebEngineLoadingInfo& info)
{
    switch (info.status()) {
        case QWebEngineLoadingInfo::LoadStartedStatus:
            if (!m_loading) {
                m_loading = true;
                emit loadingChanged(true);
            }
            if (m_progress != 0) {
                m_progress = 0;
                emit loadProgressChanged(0);
            }
            break;

        case QWebEngineLoadingInfo::LoadStoppedStatus:
        case QWebEngineLoadingInfo::LoadSucceededStatus:
            if (m_loading) {
                m_loading = false;
                emit loadingChanged(false);
            }
            if (m_progress != 100) {
                m_progress = 100;
                emit loadProgressChanged(100);
            }
            break;

        case QWebEngineLoadingInfo::LoadFailedStatus:
            if (m_loading) {
                m_loading = false;
                emit loadingChanged(false);
            }
            m_progress = 100;
            emit loadProgressChanged(100);

            // An aborted load is the user navigating away or pressing Stop,
            // not a site failure, so no error page.
            if (info.errorDomain() == QWebEngineLoadingInfo::ErrorDomain::NoErrorDomain
                || info.errorCode() == -3 /* ABORTED */) {
                break;
            }
            handleFailure(info.url(), static_cast<int>(info.errorDomain()), info.errorCode(),
                          info.errorString());
            break;
    }
}

void WebPage::onLoadFinished(bool ok)
{
    if (ok) {
        return;
    }

    // Qt 6.8 reports a main-frame failure through loadFinished(false) even in
    // cases where loadingChanged is not emitted, so this is the reliable hook.
    // The error details were already handled by onLoadingChanged if they were
    // available; here we only need the "the page did not load" case.
    if (!m_errorPageEnabled || m_showingErrorPage) {
        return;
    }
    const QUrl current = url();
    if (!m_requestedUrl.isEmpty() && current != m_requestedUrl) {
        // A redirect or a superseded request failed, not the page in front.
        return;
    }
    handleFailure(current, static_cast<int>(QWebEngineLoadingInfo::ErrorDomain::NoErrorDomain), 0,
                  QString());
}

void WebPage::onTitleChanged(const QString& title)
{
    emit titleChanged(title);
}

void WebPage::onUrlChanged(const QUrl& newUrl)
{
    emit urlChanged(newUrl);
}

void WebPage::onIconChanged()
{
    m_lastIcon = icon();
    emit iconChanged();
}

void WebPage::onFullScreenRequest(QWebEngineFullScreenRequest request)
{
    // The page asks to toggle full screen; the shell flips its window state.
    emit fullScreenRequested(request.toggleOn());
    request.accept();
}

void WebPage::onNewWindowRequested(QWebEngineNewWindowRequest& request)
{
    // The request is intentionally *not* accepted with openIn(): Yozora opens
    // a real tab instead of a Chromium window.
    const QUrl target = request.requestedUrl();
    if (target.isValid() && !target.isEmpty()) {
        emit newWindowRequested(target, request.isUserInitiated());
    }
}

void WebPage::onRenderProcessTerminated(RenderProcessTerminationStatus status, int exitCode)
{
    Q_UNUSED(status)
    Q_UNUSED(exitCode)
    // The renderer crashed. Reloading is the only sane recovery in the MVP.
    emit renderProcessTerminatedUnexpectedly(static_cast<int>(status));
    triggerAction(QWebEnginePage::Reload);
}

bool WebPage::acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame)
{
    const QString scheme = url.scheme().toLower();

    // A URL without a scheme is resolved by the engine; leave it alone.
    if (scheme.isEmpty()) {
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

    // Schemes the engine renders itself. This list is a deliberate allowlist:
    // anything not on it is treated as an external protocol.
    static const QSet<QString> webSchemes = {
        QStringLiteral("http"),       QStringLiteral("https"),
        QStringLiteral("ws"),         QStringLiteral("wss"),
        QStringLiteral("ftp"),        QStringLiteral("about"),
        QStringLiteral("data"),       QStringLiteral("blob"),
        QStringLiteral("qrc"),        QStringLiteral("yozora-error"),
        QStringLiteral("javascript"), QStringLiteral("view-source"),
    };
    if (webSchemes.contains(scheme)) {
        return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
    }

    // Web content must not be able to read the user's local files. A file URL
    // is only allowed when the user typed it (or navigated back to it), never
    // when a page linked to it, redirected to it or framed it.
    if (scheme == QLatin1String("file")) {
        const bool userInitiated =
            (type == NavigationTypeTyped || type == NavigationTypeBackForward);
        if (userInitiated && isMainFrame) {
            return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
        }
        return false;
    }

    // Everything else (mailto:, tel:, magnet:, unknown custom schemes): never
    // launched silently. The shell asks the user first.
    emit externalProtocolRequested(url, static_cast<int>(type));
    return false;
}

void WebPage::onCertificateError(const QWebEngineCertificateError& error)
{
    // Yozora never ignores a certificate error. Leaving the object untouched
    // means Chromium rejects the connection (the default), so the page fails
    // and the Yozora error page explains it. acceptCertificate() is never
    // called anywhere in the code base.
    const QByteArray url = error.url().toString().toUtf8();
    const QByteArray description = error.description().toUtf8();
    qCWarning(lcWebPage, "Rejecting certificate error for %s: %s", url.constData(),
              description.constData());
}

}  // namespace yozora
