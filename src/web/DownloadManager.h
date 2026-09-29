// SPDX-License-Identifier: MIT
#pragma once

#include <QHash>
#include <QObject>
#include <QString>

class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QLabel;
class QProgressBar;
class QWidget;

namespace yozora {

class Settings;

// Minimal download handling for the MVP: every download goes to the folder
// chosen in settings, a thin status bar appears with progress, and the user
// can open the finished file or the containing folder.
//
// This is deliberately not a download manager. When a real one is needed it
// should replace this class and keep the same interface.
class DownloadManager : public QObject {
    Q_OBJECT

public:
    DownloadManager(QWebEngineProfile* profile, Settings* settings, QWidget* parent = nullptr);
    ~DownloadManager() override;

    // The status strip; hidden when nothing is downloading.
    [[nodiscard]] QWidget* statusBar() const { return m_bar; }

    // Opens the file with the OS default application. Files that can execute
    // code (exe, msi, bat, ...) are never opened without an explicit warning.
    static void openFile(const QString& path, QWidget* parent = nullptr);

    // Opens the folder containing `path`, selecting the file when possible.
    static void revealInFolder(const QString& path);

    [[nodiscard]] int activeDownloadCount() const;

signals:
    void downloadStarted(const QString& fileName);
    void downloadFinished(const QString& filePath);
    void downloadFailed(const QString& fileName, const QString& reason);

private:
    void onDownloadRequested(QWebEngineDownloadRequest* request);
    void onStateChanged(QWebEngineDownloadRequest* request);
    void onReceivedBytesChanged(QWebEngineDownloadRequest* request);
    void onTotalBytesChanged(QWebEngineDownloadRequest* request);

    void showBar();
    void hideBarIfIdle();
    void resetBar();

    Settings* m_settings = nullptr;
    QWidget* m_bar = nullptr;
    QLabel* m_label = nullptr;
    QProgressBar* m_progress = nullptr;
    QHash<QWebEngineDownloadRequest*, QString> m_files;
};

}  // namespace yozora
