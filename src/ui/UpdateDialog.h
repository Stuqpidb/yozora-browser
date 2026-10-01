// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <QUrl>

class QLabel;
class QProgressBar;
class QPushButton;

namespace yozora {

struct ReleaseInfo;

// The window shown when an update is found, when one cannot be looked up, and
// while it is downloading.
//
// It deliberately stops at the download. The file is fetched, its size is
// checked against the number the release page published, and then it is handed
// over: this window never starts the installer itself. A browser that quietly
// launched a freshly downloaded executable would be a bad thing to ship, and "it
// only ever comes from our own releases" stops being an argument the moment the
// download is what got compromised.
class UpdateDialog : public QDialog {
    Q_OBJECT

public:
    UpdateDialog(QWidget* parent = nullptr);

    // "Version 0.3.0 is available", with the release notes if there are any.
    void offerRelease(const ReleaseInfo& info);

    // The reason a check failed, in words rather than as a status code.
    void showProblem(const QString& reason);

    void beginDownload(const QString& fileName, qint64 totalBytes);
    void setProgress(qint64 received, qint64 totalBytes);
    void finishDownload(const QString& path, bool sizeMatched);
    void failDownload(const QString& reason);

signals:
    // The user pressed Download. The window does not fetch anything itself.
    void downloadRequested();
    // The user wants to be taken to the release page instead.
    void openPageRequested(const QUrl& url);
    // The user pressed "Run the installer". The window still does not run it.
    void runInstallerRequested(const QString& path);
    void cancelRequested();

private:
    enum class State {
        Offer,       // an update was found, nothing downloaded yet
        Downloading,
        Downloaded,
        Failed,      // the check itself failed, or the download did
    };

    void showState(State state);

    QLabel* m_heading = nullptr;
    QLabel* m_notes = nullptr;
    QProgressBar* m_progress = nullptr;
    QPushButton* m_downloadButton = nullptr;
    QPushButton* m_pageButton = nullptr;
    QPushButton* m_runButton = nullptr;
    QPushButton* m_closeButton = nullptr;
    QUrl m_pageUrl;
    QString m_path;
};

}  // namespace yozora
