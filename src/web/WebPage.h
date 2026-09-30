// SPDX-License-Identifier: MIT
#pragma once

#include <QIcon>
#include <QUrl>
#include <QWebEngineLoadingInfo>
#include <QWebEnginePage>

class QWebEngineProfile;
class QWebEngineFullScreenRequest;
class QWebEngineNewWindowRequest;
class QWebEngineCertificateError;

namespace yozora {

// QWebEnginePage subclass that turns the raw Qt WebEngine API into a small,
// explicit set of signals the browser shell can rely on.
//
// Security note: the page deliberately does not loosen any default. In
// particular certificateError() rejects invalid certificates, and the
// Chromium error page is replaced by Yozora's own.
class WebPage : public QWebEnginePage {
    Q_OBJECT

public:
    explicit WebPage(QWebEngineProfile* profile, QObject* parent = nullptr);

    // When true (default) a failing main-frame load shows Yozora's own error
    // page instead of the Chromium one.
    void setErrorPageEnabled(bool enabled);
    [[nodiscard]] bool errorPageEnabled() const { return m_errorPageEnabled; }


    // Loads `url`, showing Yozora's error page if it cannot be reached.
    void loadUrl(const QUrl& url);

    // Renders the Yozora error document in the current view.
    void showErrorPage(const QUrl& url, int errorDomain, int errorCode, const QString& errorText);

    // The address the current error page is complaining about, plus the title
    // that page shows. The address bar and the tab label use these, because the
    // document itself has already been replaced by the error page.
    [[nodiscard]] QUrl errorPageFor() const { return m_errorUrl; }
    [[nodiscard]] QString errorTitle() const { return m_errorTitle; }
    [[nodiscard]] bool showingErrorPage() const { return m_showingErrorPage; }

    [[nodiscard]] bool isLoading() const { return m_loading; }
    [[nodiscard]] int loadProgress() const { return m_progress; }

    // Schemes that are not real web documents (the start page, error pages,
    // inline data). Used to decide what may appear in the address bar.
    static bool isTransientScheme(const QUrl& url);

signals:
    void urlChanged(const QUrl& url);
    void titleChanged(const QString& title);
    void iconChanged();
    void loadingChanged(bool loading);
    void loadProgressChanged(int percent);

    // window.open() / target=_blank: the shell turns this into a new tab.
    void newWindowRequested(const QUrl& url, bool foreground);

    // A link or page asked to open a scheme Yozora does not render itself
    // (mailto:, tel:, magnet:, or anything unknown). The shell decides whether
    // to hand it to the operating system; the engine never does so silently.
    void externalProtocolRequested(const QUrl& url, int navigationType);

    void linkHovered(const QString& url);
    void fullScreenRequested(bool enabled);
    void renderProcessTerminatedUnexpectedly(int status);

protected:
    // Security gate for every navigation. Allows the schemes the engine renders
    // itself, refuses file:// loaded by web content, and routes every other
    // scheme to the shell instead of launching it silently.
    bool acceptNavigationRequest(const QUrl& url, NavigationType type, bool isMainFrame) override;

private:
    void onLoadingChanged(const QWebEngineLoadingInfo& info);
    void onLoadFinished(bool ok);
    void onTitleChanged(const QString& title);
    void onUrlChanged(const QUrl& url);
    void onIconChanged();
    void onFullScreenRequest(QWebEngineFullScreenRequest request);
    void onNewWindowRequested(QWebEngineNewWindowRequest& request);
    void onRenderProcessTerminated(RenderProcessTerminationStatus status, int exitCode);
    void onCertificateError(const QWebEngineCertificateError& error);

    // Decides whether a failed load deserves a Yozora error page, and shows it.
    void handleFailure(const QUrl& url, int errorDomain, int errorCode,
                       const QString& errorText);

    QIcon m_lastIcon;
    QUrl m_requestedUrl;
    QUrl m_errorUrl;
    QString m_errorTitle;
    bool m_errorPageEnabled = true;
    bool m_loading = false;
    bool m_showingErrorPage = false;
    int m_progress = 0;
};

}  // namespace yozora
