// SPDX-License-Identifier: MIT
#include "web/WebView.h"

#include "web/WebPage.h"

#include <QAction>
#include <QClipboard>
#include <QColor>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QWheelEvent>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#include <cmath>

namespace yozora {

WebView::WebView(QWebEngineProfile* profile, QWidget* parent)
    : QWebEngineView(parent)
{
    setPage(new WebPage(profile, this));
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setFocusPolicy(Qt::StrongFocus);

    settings()->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);

    // Drives the custom "Fast" wheel animation. ~60 Hz keeps it smooth without
    // flooding the renderer.
    m_smoothTimer = new QTimer(this);
    m_smoothTimer->setInterval(15);
    connect(m_smoothTimer, &QTimer::timeout, this, &WebView::stepSmoothScroll);
}

WebView::~WebView() = default;

void WebView::applyDarkPalette()
{
    // The renderer is a separate process with its own idea of colours, so the
    // window palette has to be pushed into the view explicitly. Yozora has one
    // theme, so this is set once instead of being toggled.
    auto viewPalette = palette();
    viewPalette.setColor(QPalette::Base, QColor(0x1a, 0x21, 0x2c));
    viewPalette.setColor(QPalette::Text, QColor(0xe6, 0xea, 0xf2));
    viewPalette.setColor(QPalette::Window, QColor(0x0d, 0x10, 0x17));
    viewPalette.setColor(QPalette::WindowText, QColor(0xe6, 0xea, 0xf2));
    setPalette(viewPalette);
}

void WebView::setScrollMode(Settings::ScrollMode mode)
{
    m_scrollMode = mode;
    if (m_scrollMode != Settings::ScrollMode::Fast) {
        m_pendingScroll = QPointF();
        if (m_smoothTimer) {
            m_smoothTimer->stop();
        }
    }
}

void WebView::wheelEvent(QWheelEvent* event)
{
    // Only the custom "Fast" mode is handled here. Instant and Smooth are left
    // entirely to the engine.
    if (m_scrollMode != Settings::ScrollMode::Fast) {
        QWebEngineView::wheelEvent(event);
        return;
    }

    // High-resolution devices (trackpads, precision wheels) already deliver
    // pixel deltas that feel smooth; leave them alone.
    if (!event->pixelDelta().isNull()) {
        QWebEngineView::wheelEvent(event);
        return;
    }

    // Never interfere with modifier shortcuts such as Ctrl+wheel zoom.
    if (event->modifiers() != Qt::NoModifier || event->angleDelta().isNull()) {
        QWebEngineView::wheelEvent(event);
        return;
    }

    // A notch is 120 eighths of a degree; map it to a comfortable pixel step.
    constexpr qreal kPixelsPerNotch = 105.0;
    const QPoint angle = event->angleDelta();
    m_pendingScroll += QPointF(angle.x() / 120.0 * kPixelsPerNotch,
                               angle.y() / 120.0 * kPixelsPerNotch);
    m_lastWheelPos = event->position();
    m_lastWheelButtons = event->buttons();
    m_lastWheelModifiers = event->modifiers();
    event->accept();

    if (!m_smoothTimer->isActive()) {
        m_smoothTimer->start();
    }
}

void WebView::stepSmoothScroll()
{
    if (m_pendingScroll.isNull()) {
        m_pendingScroll = QPointF();
        m_smoothTimer->stop();
        return;
    }

    // Exponential ease-out: every frame covers a fixed share of what is left,
    // so the motion starts quickly and settles instead of stopping dead.
    QPointF step = m_pendingScroll * 0.38;
    if (std::abs(step.x()) < 1.0 && m_pendingScroll.x() != 0.0) {
        step.setX(m_pendingScroll.x());
    }
    if (std::abs(step.y()) < 1.0 && m_pendingScroll.y() != 0.0) {
        step.setY(m_pendingScroll.y());
    }

    const QPoint pixels(static_cast<int>(std::lround(step.x())),
                        static_cast<int>(std::lround(step.y())));
    m_pendingScroll -= QPointF(pixels.x(), pixels.y());

    if (pixels.isNull()) {
        m_pendingScroll = QPointF();
        m_smoothTimer->stop();
        return;
    }

    // Re-emitting the wheel with a pixel delta produces a short, controlled
    // scroll without touching the page: the engine only ever sees ordinary
    // wheel input.
    QWheelEvent synthetic(m_lastWheelPos, mapToGlobal(m_lastWheelPos.toPoint()),
                          pixels, QPoint(), m_lastWheelButtons, m_lastWheelModifiers,
                          Qt::NoScrollPhase, false);
    QWebEngineView::wheelEvent(&synthetic);
}

void WebView::contextMenuEvent(QContextMenuEvent* event)
{
    // Qt 6.8 hands the menu data over as a request object; it stays valid only
    // for the duration of this event, so the menu is built synchronously.
    buildContextMenu(lastContextMenuRequest(), mapToGlobal(event->pos()));
    event->accept();
}

void WebView::buildContextMenu(QWebEngineContextMenuRequest* request, const QPoint& globalPos)
{
    QMenu menu(this);
    addNavigationActions(&menu);

    if (!request) {
        menu.exec(globalPos);
        return;
    }

    const bool editable = request->isContentEditable();

    if (editable) {
        menu.addSeparator();
        addClipboardActions(&menu, request);
    } else {
        if (!request->selectedText().isEmpty()) {
            menu.addSeparator();
            QAction* copy = menu.addAction(tr("Copy"));
            connect(copy, &QAction::triggered, this, [this] {
                page()->triggerAction(QWebEnginePage::Copy);
                showStatusMessage(tr("Copied to clipboard"));
            });
        }
        if (request->linkUrl().isValid()) {
            menu.addSeparator();
            addLinkActions(&menu, request->linkUrl());
        }
    }

    if (request->mediaType() == QWebEngineContextMenuRequest::MediaTypeImage
        && request->mediaUrl().isValid()) {
        if (menu.actions().size() > 1) {
            menu.addSeparator();
        }
        addImageActions(&menu, request->mediaUrl());
    }

    if (menu.isEmpty()) {
        return;
    }

    menu.exec(globalPos);
}

void WebView::addNavigationActions(QMenu* menu)
{
    QAction* back = menu->addAction(tr("Back"));
    back->setEnabled(page()->history()->canGoBack());
    connect(back, &QAction::triggered, this, [this] { page()->triggerAction(QWebEnginePage::Back); });

    QAction* forward = menu->addAction(tr("Forward"));
    forward->setEnabled(page()->history()->canGoForward());
    connect(forward, &QAction::triggered, this,
            [this] { page()->triggerAction(QWebEnginePage::Forward); });

    menu->addSeparator();

    QAction* reload = menu->addAction(tr("Reload"));
    reload->setEnabled(!page()->isLoading());
    connect(reload, &QAction::triggered, this,
            [this] { page()->triggerAction(QWebEnginePage::Reload); });

    QAction* stop = menu->addAction(tr("Stop"));
    stop->setEnabled(page()->isLoading());
    connect(stop, &QAction::triggered, this, [this] { page()->triggerAction(QWebEnginePage::Stop); });
}

void WebView::addClipboardActions(QMenu* menu, QWebEngineContextMenuRequest* request)
{
    const auto flags = request->editFlags();

    if (flags.testFlag(QWebEngineContextMenuRequest::CanCut)) {
        QAction* cut = menu->addAction(tr("Cut"));
        connect(cut, &QAction::triggered, this, [this] { page()->triggerAction(QWebEnginePage::Cut); });
    }
    if (flags.testFlag(QWebEngineContextMenuRequest::CanCopy)) {
        QAction* copy = menu->addAction(tr("Copy"));
        connect(copy, &QAction::triggered, this,
                [this] { page()->triggerAction(QWebEnginePage::Copy); });
    }
    if (flags.testFlag(QWebEngineContextMenuRequest::CanPaste)) {
        QAction* paste = menu->addAction(tr("Paste"));
        connect(paste, &QAction::triggered, this,
                [this] { page()->triggerAction(QWebEnginePage::Paste); });
    }

    menu->addSeparator();

    if (flags.testFlag(QWebEngineContextMenuRequest::CanSelectAll)) {
        QAction* selectAll = menu->addAction(tr("Select All"));
        connect(selectAll, &QAction::triggered, this,
                [this] { page()->triggerAction(QWebEnginePage::SelectAll); });
    }
}

void WebView::addLinkActions(QMenu* menu, const QUrl& link)
{
    QAction* openNewTab = menu->addAction(tr("Open Link in New Tab"));
    connect(openNewTab, &QAction::triggered, this,
            [this, link] { emit newTabRequested(link, true); });

    QAction* copyLink = menu->addAction(tr("Copy Link Address"));
    connect(copyLink, &QAction::triggered, this, [this, link] {
        QGuiApplication::clipboard()->setText(link.toString());
        showStatusMessage(tr("Link address copied"));
    });

    QAction* openLink = menu->addAction(tr("Open Link"));
    connect(openLink, &QAction::triggered, this, [this, link] { page()->load(link); });
}

void WebView::addImageActions(QMenu* menu, const QUrl& image)
{
    QAction* openImage = menu->addAction(tr("Open Image"));
    connect(openImage, &QAction::triggered, this,
            [this, image] { emit newTabRequested(image, true); });

    QAction* copyImage = menu->addAction(tr("Copy Image Address"));
    connect(copyImage, &QAction::triggered, this, [this, image] {
        QGuiApplication::clipboard()->setText(image.toString());
        showStatusMessage(tr("Image address copied"));
    });

    QAction* saveImage = menu->addAction(tr("Save Image As..."));
    connect(saveImage, &QAction::triggered, this, [this, image] {
        const QString downloads =
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        const QString fileName = QFileInfo(image.path()).fileName();
        const QString suggested =
            fileName.isEmpty() ? downloads + QStringLiteral("/image")
                               : downloads + QLatin1Char('/') + fileName;

        const QString target =
            QFileDialog::getSaveFileName(this, tr("Save Image As..."), suggested);
        if (target.isEmpty()) {
            return;
        }
        saveUrlToFile(image, target);
        showStatusMessage(tr("Downloading image..."));
    });
}

void WebView::saveUrlToFile(const QUrl& url, const QString& targetPath)
{
    if (url.isEmpty() || targetPath.isEmpty()) {
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    auto* manager = new QNetworkAccessManager(this);
    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, targetPath] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            showStatusMessage(tr("Could not download the file"));
            return;
        }
        QFile file(targetPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            showStatusMessage(tr("Could not write the file"));
            return;
        }
        file.write(reply->readAll());
        file.close();
        showStatusMessage(tr("Image saved"));
    });
}

void WebView::showStatusMessage(const QString& message)
{
    emit statusMessage(message);
}

}  // namespace yozora
