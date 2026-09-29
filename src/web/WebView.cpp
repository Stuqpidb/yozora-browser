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
#include <QUrl>
#include <QWebEngineContextMenuRequest>
#include <QWebEngineHistory>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

namespace yozora {

WebView::WebView(QWebEngineProfile* profile, QWidget* parent)
    : QWebEngineView(parent)
{
    setPage(new WebPage(profile, this));
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setFocusPolicy(Qt::StrongFocus);

    settings()->setAttribute(QWebEngineSettings::ShowScrollBars, true);
    settings()->setAttribute(QWebEngineSettings::FullScreenSupportEnabled, true);
}

WebView::~WebView() = default;

void WebView::setDarkMode(bool dark)
{
    auto viewPalette = palette();
    if (dark) {
        viewPalette.setColor(QPalette::Base, QColor(0x1a, 0x21, 0x2c));
        viewPalette.setColor(QPalette::Text, QColor(0xe6, 0xea, 0xf2));
        viewPalette.setColor(QPalette::Window, QColor(0x0d, 0x10, 0x17));
        viewPalette.setColor(QPalette::WindowText, QColor(0xe6, 0xea, 0xf2));
    } else {
        viewPalette.setColor(QPalette::Base, QColor(0xff, 0xff, 0xff));
        viewPalette.setColor(QPalette::Text, QColor(0x1a, 0x1f, 0x2b));
        viewPalette.setColor(QPalette::Window, QColor(0xf4, 0xf6, 0xfa));
        viewPalette.setColor(QPalette::WindowText, QColor(0x1a, 0x1f, 0x2b));
    }
    setPalette(viewPalette);
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
