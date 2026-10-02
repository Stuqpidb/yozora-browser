// SPDX-License-Identifier: MIT
//
// Yozora Browser - application entry point.
//
// This file only wires things together: it must stay free of browser logic so
// that main.cpp never becomes the place where features grow.

#include "app/AppPaths.h"
#include "browser/BrowserWindow.h"
#include "core/BookmarkStore.h"
#include "core/HistoryStore.h"
#include "core/Settings.h"
#include "core/Theme.h"
#include "utils/Version.h"
#include "web/WebProfile.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QLibraryInfo>
#include <QLockFile>
#include <QMessageBox>
#include <QScopedPointer>
#include <QSize>
#include <QUrl>

using namespace yozora;

namespace {

// Only one browser instance may own the profile directory at a time; two
// Chromium processes writing the same cookies database is a corruption bug.
QLockFile* acquireSingleInstanceLock()
{
    const QString lockPath =
        QDir(AppPaths::userDataDir()).filePath(QStringLiteral("instance.lock"));
    auto* lock = new QLockFile(lockPath);
    lock->setStaleLockTime(0);
    if (!lock->tryLock(100)) {
        return nullptr;
    }
    return lock;
}

QIcon applicationIcon()
{
    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/yozora-16.png"), QSize(16, 16));
    icon.addFile(QStringLiteral(":/icons/yozora-24.png"), QSize(24, 24));
    icon.addFile(QStringLiteral(":/icons/yozora-32.png"), QSize(32, 32));
    icon.addFile(QStringLiteral(":/icons/yozora-48.png"), QSize(48, 48));
    icon.addFile(QStringLiteral(":/icons/yozora-64.png"), QSize(64, 64));
    icon.addFile(QStringLiteral(":/icons/yozora-256.png"), QSize(256, 256));
    return icon;
}

// The WebRTC routing policy has to be handed to Chromium through an environment
// variable, which only Qt WebEngine reads at startup, before any profile
// exists. This is why it is read directly from the store here rather than
// through the Settings object (which needs QApplication).
void applyStartupPrivacyFlags()
{
    const Settings::WebRtcPolicy policy = Settings::bootWebRtcPolicy();
    if (policy == Settings::WebRtcPolicy::Default) {
        return;
    }
    const char* value = policy == Settings::WebRtcPolicy::PublicInterfaceOnly
                            ? "default_public_interface_only"
                            : "disable_non_proxied_udp";

    QByteArray flags = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    if (!flags.isEmpty() && !flags.endsWith(' ')) {
        flags.append(' ');
    }
    flags.append("--force-webrtc-ip-handling-policy=");
    flags.append(value);
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", flags);
}

}  // namespace

int main(int argc, char* argv[])
{
    // This must happen before QApplication so Qt WebEngine picks it up when it
    // initialises Chromium.
    applyStartupPrivacyFlags();

    // Required by Qt WebEngine: the OpenGL context must be shared between the
    // widgets and the GPU process. Must be set before QApplication exists.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    QApplication app(argc, argv);
    app.setApplicationName(QString::fromLatin1(kDisplayName));
    app.setApplicationDisplayName(QString::fromLatin1(kDisplayName));
    app.setApplicationVersion(QString::fromLatin1(kVersionString));
    app.setOrganizationName(QString::fromLatin1(kOrganization));
    app.setOrganizationDomain(QString::fromLatin1(kDomain));
    app.setDesktopFileName(QStringLiteral("org.yozora.browser"));
    app.setWindowIcon(applicationIcon());

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Yozora Browser - a native desktop browser with a Chromium engine."));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption privateOption(
        QStringLiteral("private"),
        QStringLiteral("Open a private browsing window in addition to the main window."));
    parser.addOption(privateOption);
    parser.addPositionalArgument(
        QStringLiteral("url"), QStringLiteral("Address to open on start."));

    parser.process(app);

    AppPaths::ensureCreated();

    QScopedPointer<QLockFile> lock(acquireSingleInstanceLock());
    if (lock.isNull()) {
        QMessageBox::information(
            nullptr, QStringLiteral("Yozora"),
            QStringLiteral("Yozora is already running. Open Yozora from the taskbar or the "
                           "Start menu to use it."));
        return 0;
    }
    lock->setStaleLockTime(30000);

    // A deferred "clear site storage" request runs here, before any profile is
    // created, so Chromium cannot be writing the files that are removed.
    WebProfile::purgeSiteStorageIfRequested();

    Settings settings;
    Theme::apply();

    WebProfile profile(&settings);
    BookmarkStore bookmarks;
    HistoryStore history;

    BrowserWindow window(&profile, &settings, &bookmarks, &history);

    // An address on the command line replaces the start page in the first tab
    // instead of opening a second one.
    const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        window.openInFirstTab(QUrl::fromUserInput(positional.first()));
    } else if (settings.restoreSessionOnStart() || window.sessionCrashed()) {
        // Reopen the previous tabs: either because the user asked for it, or
        // because the last run did not exit cleanly and this is a recovery.
        window.restoreSession();
    }
    window.show();

    if (parser.isSet(privateOption)) {
        window.openPrivateWindow();
    }

    return app.exec();
}
