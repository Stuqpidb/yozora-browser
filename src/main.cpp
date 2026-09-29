// SPDX-License-Identifier: MIT
//
// Yozora Browser - application entry point.
//
// This file only wires things together: it must stay free of browser logic so
// that main.cpp never becomes the place where features grow.

#include "app/AppPaths.h"
#include "browser/BrowserWindow.h"
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

}  // namespace

int main(int argc, char* argv[])
{
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

    Settings settings;
    Theme::apply(settings.themeMode() != Settings::ThemeMode::Light);

    WebProfile profile;

    BrowserWindow window(&profile, &settings);

    // An address on the command line replaces the start page in the first tab
    // instead of opening a second one.
    const QStringList positional = parser.positionalArguments();
    if (!positional.isEmpty()) {
        window.openInFirstTab(QUrl::fromUserInput(positional.first()));
    }
    window.show();

    return app.exec();
}
