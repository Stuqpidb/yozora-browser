// SPDX-License-Identifier: MIT
#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QWebEngineProfile;
class QWebEngineDownloadRequest;
class QLabel;
class QProgressBar;
class QWidget;

namespace yozora {

class Settings;

// One row in the download history the manager keeps for the session, so the
// downloads window has something to show beyond the transient status strip.
struct DownloadRecord {
    enum class State { Active, Completed, Failed };
    QString fileName;
    QString path;
    qint64 receivedBytes = 0;
    qint64 totalBytes = 0;
    State state = State::Active;
};

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

    // Session history, newest first, for the downloads window.
    [[nodiscard]] QList<DownloadRecord> records() const { return m_records; }
    void clearHistory();

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
    // The history changed (a download started, progressed, finished or failed).
    void recordsChanged();

private:
    void onDownloadRequested(QWebEngineDownloadRequest* request);
    void onStateChanged(QWebEngineDownloadRequest* request);
    void onReceivedBytesChanged(QWebEngineDownloadRequest* request);
    void onTotalBytesChanged(QWebEngineDownloadRequest* request);

    void showBar();
    void hideBarIfIdle();
    void resetBar();
    void markRecord(const QString& path, DownloadRecord::State state, qint64 received,
                    qint64 total);

    Settings* m_settings = nullptr;
    QWidget* m_bar = nullptr;
    QLabel* m_label = nullptr;
    QProgressBar* m_progress = nullptr;
    QHash<QWebEngineDownloadRequest*, QString> m_files;
    QList<DownloadRecord> m_records;
};

}  // namespace yozora
