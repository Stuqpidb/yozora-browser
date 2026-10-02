// SPDX-License-Identifier: MIT
#include "ui/DownloadsDialog.h"

#include "web/DownloadManager.h"

#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace yozora {

DownloadsDialog::DownloadsDialog(DownloadManager* manager, QWidget* parent)
    : QDialog(parent)
    , m_manager(manager)
{
    setWindowTitle(tr("Downloads"));
    resize(560, 400);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 12);
    root->setSpacing(10);

    auto* title = new QLabel(tr("Downloads"), this);
    title->setObjectName(QStringLiteral("dialogTitle"));
    root->addWidget(title);

    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("libraryList"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_list, 1);

    auto* buttons = new QHBoxLayout;
    m_openButton = new QPushButton(tr("Open"), this);
    m_folderButton = new QPushButton(tr("Show in folder"), this);
    auto* clearButton = new QPushButton(tr("Clear history"), this);
    buttons->addWidget(m_openButton);
    buttons->addWidget(m_folderButton);
    buttons->addStretch(1);
    buttons->addWidget(clearButton);
    root->addLayout(buttons);

    connect(clearButton, &QPushButton::clicked, this, [this] {
        if (m_manager) {
            m_manager->clearHistory();
        }
    });

    const auto selectedPath = [this]() -> QString {
        if (auto* item = m_list->currentItem()) {
            return item->data(Qt::UserRole).toString();
        }
        return QString();
    };

    connect(m_openButton, &QPushButton::clicked, this, [this, selectedPath] {
        DownloadManager::openFile(selectedPath(), this);
    });
    connect(m_folderButton, &QPushButton::clicked, this,
            [selectedPath] { DownloadManager::revealInFolder(selectedPath()); });
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this, selectedPath] {
        DownloadManager::openFile(selectedPath(), this);
    });

    if (m_manager) {
        connect(m_manager, &DownloadManager::recordsChanged, this, &DownloadsDialog::refresh);
    }
    refresh();
}

void DownloadsDialog::refresh()
{
    m_list->clear();
    if (!m_manager) {
        return;
    }

    const QList<DownloadRecord> records = m_manager->records();
    for (const DownloadRecord& record : records) {
        QString detail;
        switch (record.state) {
            case DownloadRecord::State::Active: {
                const int percent = record.totalBytes > 0
                                        ? static_cast<int>(record.receivedBytes * 100
                                                           / record.totalBytes)
                                        : 0;
                detail = tr("%1% - downloading").arg(percent);
                break;
            }
            case DownloadRecord::State::Completed:
                detail = tr("Completed");
                break;
            case DownloadRecord::State::Failed:
                detail = tr("Failed");
                break;
        }
        auto* item = new QListWidgetItem(QStringLiteral("%1\n%2").arg(record.fileName, detail),
                                         m_list);
        item->setData(Qt::UserRole, record.path);
    }

    if (records.isEmpty()) {
        auto* item = new QListWidgetItem(tr("No downloads yet."), m_list);
        item->setFlags(Qt::NoItemFlags);
    }

    m_openButton->setEnabled(!records.isEmpty());
    m_folderButton->setEnabled(!records.isEmpty());
}

}  // namespace yozora
