// SPDX-License-Identifier: MIT
#pragma once

#include <QPoint>
#include <QWebEngineView>

class QMenu;
class QWebEngineProfile;
class QWebEngineContextMenuRequest;

namespace yozora {

// The widget that hosts a page, plus the page context menu.
//
// Everything Chromium-specific lives here; the rest of the application only
// ever talks to WebPage through signals.
class WebView : public QWebEngineView {
    Q_OBJECT

public:
    explicit WebView(QWebEngineProfile* profile, QWidget* parent = nullptr);
    ~WebView() override;

    // Fetches `url` and writes the bytes to `targetPath` (used by
    // "Save image as...").
    void saveUrlToFile(const QUrl& url, const QString& targetPath);

    // Applies the Yozora palette to the widget, so form controls and
    // scrollbars match the shell theme.
    void setDarkMode(bool dark);

signals:
    // Raised by context-menu actions that belong to the shell, not the page.
    void newTabRequested(const QUrl& url, bool foreground);
    void statusMessage(const QString& message);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    void buildContextMenu(QWebEngineContextMenuRequest* request, const QPoint& globalPos);
    void addNavigationActions(QMenu* menu);
    void addClipboardActions(QMenu* menu, QWebEngineContextMenuRequest* request);
    void addLinkActions(QMenu* menu, const QUrl& link);
    void addImageActions(QMenu* menu, const QUrl& image);

    void showStatusMessage(const QString& message);
};

}  // namespace yozora
