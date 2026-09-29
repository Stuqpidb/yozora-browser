// SPDX-License-Identifier: MIT
#include "web/DownloadManager.h"

#include "core/Settings.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QToolButton>
#include <QUrl>
#include <QWebEngineDownloadRequest>
#include <QWebEngineProfile>
#include <QWidget>

namespace yozora {

DownloadManager::DownloadManager(QWebEngineProfile* profile, yozora::Settings* settings,
                                 QWidget* parent)
    : QObject(parent)
    , m_settings(settings)
{
    m_bar = new QWidget(parent);
    m_bar->setObjectName(QStringLiteral("downloadBar"));
    m_bar->setVisible(false);

    auto* layout = new QHBoxLayout(m_bar);
    layout->setContentsMargins(12, 6, 12, 6);
    layout->setSpacing(10);

    m_label = new QLabel(m_bar);
    m_label->setObjectName(QStringLiteral("downloadLabel"));
    layout->addWidget(m_label);

    m_progress = new QProgressBar(m_bar);
    m_progress->setObjectName(QStringLiteral("downloadProgress"));
    m_progress->setFixedWidth(140);
    m_progress->setTextVisible(false);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    auto* openButton = new QToolButton(m_bar);
    openButton->setObjectName(QStringLiteral("downloadOpenButton"));
    openButton->setText(tr("Open"));
    openButton->setVisible(false);
    connect(openButton, &QToolButton::clicked, this, [this] {
        for (auto it = m_files.cbegin(); it != m_files.cend(); ++it) {
            openFile(it.value());
            break;
        }
    });
    layout->addWidget(openButton);

    auto* folderButton = new QToolButton(m_bar);
    folderButton->setText(tr("Folder"));
    connect(folderButton, &QToolButton::clicked, this, [this] {
        for (auto it = m_files.cbegin(); it != m_files.cend(); ++it) {
            revealInFolder(it.value());
            break;
        }
    });
    layout->addWidget(folderButton);

    auto* closeButton = new QToolButton(m_bar);
    closeButton->setText(QStringLiteral("✕"));
    closeButton->setToolTip(tr("Hide"));
    connect(closeButton, &QToolButton::clicked, this, &DownloadManager::resetBar);
    layout->addWidget(closeButton);

    layout->addStretch(1);

    connect(profile, &QWebEngineProfile::downloadRequested, this,
            &DownloadManager::onDownloadRequested);
}

DownloadManager::~DownloadManager()
{
    for (auto it = m_files.cbegin(); it != m_files.cend(); ++it) {
        it.key()->cancel();
    }
    m_files.clear();
}

int DownloadManager::activeDownloadCount() const
{
    return static_cast<int>(m_files.size());
}

void DownloadManager::onDownloadRequested(QWebEngineDownloadRequest* request)
{
    QString directory = m_settings ? m_settings->downloadDirectory() : QString();
    if (directory.isEmpty()) {
        directory = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    }

    QString fileName = request->downloadFileName();
    if (fileName.isEmpty()) {
        fileName = request->suggestedFileName();
    }
    if (fileName.isEmpty()) {
        fileName = QStringLiteral("download");
    }

    request->setDownloadDirectory(directory);
    request->setDownloadFileName(fileName);

    m_files.insert(request, directory + QLatin1Char('/') + fileName);

    connect(request, &QWebEngineDownloadRequest::stateChanged, this,
            [this, request](QWebEngineDownloadRequest::DownloadState) {
                onStateChanged(request);
            });
    connect(request, &QWebEngineDownloadRequest::receivedBytesChanged, this,
            [this, request] { onReceivedBytesChanged(request); });
    connect(request, &QWebEngineDownloadRequest::totalBytesChanged, this,
            [this, request] { onTotalBytesChanged(request); });

    m_label->setText(tr("Downloading %1...").arg(fileName));
    m_progress->setVisible(true);
    m_progress->setRange(0, 0);  // indeterminate until the total size is known
    showBar();

    emit downloadStarted(fileName);
    request->accept();
}

void DownloadManager::onStateChanged(QWebEngineDownloadRequest* request)
{
    const QString path = m_files.value(request);
    const QString name = QFileInfo(path).fileName();

    switch (request->state()) {
        case QWebEngineDownloadRequest::DownloadCompleted: {
            m_label->setText(tr("%1 - Download complete").arg(name));
            m_progress->setVisible(false);
            emit downloadFinished(path);
            m_files.remove(request);
            request->deleteLater();
            hideBarIfIdle();
            break;
        }
        case QWebEngineDownloadRequest::DownloadCancelled:
        case QWebEngineDownloadRequest::DownloadInterrupted: {
            const QString reason = request->interruptReasonString();
            m_label->setText(tr("%1 - Download failed").arg(name));
            m_progress->setVisible(false);
            emit downloadFailed(name, reason);
            m_files.remove(request);
            request->deleteLater();
            hideBarIfIdle();
            break;
        }
        default:
            break;
    }
}

void DownloadManager::onReceivedBytesChanged(QWebEngineDownloadRequest* request)
{
    if (m_progress->maximum() > 0) {
        m_progress->setValue(static_cast<int>(request->receivedBytes()));
    }
}

void DownloadManager::onTotalBytesChanged(QWebEngineDownloadRequest* request)
{
    const qint64 total = request->totalBytes();
    if (total > 0) {
        m_progress->setRange(0, 100);
        m_progress->setValue(static_cast<int>(request->receivedBytes() * 100 / total));
    }
}

void DownloadManager::showBar()
{
    m_bar->setVisible(true);
}

void DownloadManager::hideBarIfIdle()
{
    if (m_files.isEmpty()) {
        m_bar->setVisible(false);
    }
}

void DownloadManager::resetBar()
{
    m_bar->setVisible(false);
    m_label->clear();
    m_progress->setValue(0);
}

void DownloadManager::openFile(const QString& path)
{
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void DownloadManager::revealInFolder(const QString& path)
{
    if (path.isEmpty()) {
        return;
    }
    const QFileInfo info(path);
    const QString folder = info.absolutePath();
    if (!QFileInfo::exists(folder)) {
        return;
    }

#if defined(Q_OS_WIN)
    // explorer /select highlights the file, which is what users expect.
    QProcess::startDetached(QStringLiteral("explorer.exe"),
                            {QStringLiteral("/select,") + QDir::toNativeSeparators(path)});
#elif defined(Q_OS_MACOS)
    QProcess::startDetached(QStringLiteral("open"), {folder});
#else
    QProcess::startDetached(QStringLiteral("xdg-open"), {folder});
#endif
}

}  // namespace yozora
