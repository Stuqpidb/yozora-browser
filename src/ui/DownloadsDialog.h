// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>

class QListWidget;
class QPushButton;

namespace yozora {

class DownloadManager;

// The downloads window: the session's downloads, newest first, with progress,
// and Open / Show in folder for the finished ones. The status strip under the
// page only ever shows one download; this is where the rest live.
class DownloadsDialog : public QDialog {
    Q_OBJECT

public:
    DownloadsDialog(DownloadManager* manager, QWidget* parent = nullptr);

private:
    void refresh();

    DownloadManager* m_manager = nullptr;
    QListWidget* m_list = nullptr;
    QPushButton* m_openButton = nullptr;
    QPushButton* m_folderButton = nullptr;
};

}  // namespace yozora
