// SPDX-License-Identifier: MIT
#pragma once

#include "core/Settings.h"

#include <QPoint>
#include <QPointF>
#include <QWebEngineView>

class QMenu;
class QTimer;
class QWheelEvent;
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


    // Selects how the mouse wheel scrolls. Only Settings::ScrollMode::Fast
    // makes this class animate anything; the other modes are handled by the
    // engine.
    void setScrollMode(Settings::ScrollMode mode);

signals:
    // Raised by context-menu actions that belong to the shell, not the page.
    void newTabRequested(const QUrl& url, bool foreground);
    void statusMessage(const QString& message);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // Applies the Yozora palette to the widget, so form controls and scrollbars
    // match the shell.
    void applyDarkPalette();

    void buildContextMenu(QWebEngineContextMenuRequest* request, const QPoint& globalPos);
    void addNavigationActions(QMenu* menu);
    void addClipboardActions(QMenu* menu, QWebEngineContextMenuRequest* request);
    void addLinkActions(QMenu* menu, const QUrl& link);
    void addImageActions(QMenu* menu, const QUrl& image);

    // Advances the custom "Fast" wheel animation by one frame.
    void stepSmoothScroll();

    void showStatusMessage(const QString& message);

    Settings::ScrollMode m_scrollMode = Settings::ScrollMode::Fast;
    QTimer* m_smoothTimer = nullptr;
    QPointF m_pendingScroll;      // pixels still to scroll (x, y)
    QPointF m_lastWheelPos;
    Qt::MouseButtons m_lastWheelButtons = Qt::NoButton;
    Qt::KeyboardModifiers m_lastWheelModifiers = Qt::NoModifier;
};

}  // namespace yozora
