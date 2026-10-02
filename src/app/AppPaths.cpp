// SPDX-License-Identifier: MIT
#include "app/AppPaths.h"

#include "utils/Version.h"

#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>

namespace yozora {

QString AppPaths::userDataDir()
{
    const QString base =
        QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (base.isEmpty()) {
        return QDir::homePath() + QStringLiteral("/.yozora");
    }
    return base;
}

QString AppPaths::profileDir()
{
    return userDataDir() + QStringLiteral("/profile");
}

QString AppPaths::logsDir()
{
    return userDataDir() + QStringLiteral("/logs");
}

QString AppPaths::privacyDir()
{
    return userDataDir() + QStringLiteral("/privacy");
}

QString AppPaths::trackerListPath()
{
    return privacyDir() + QStringLiteral("/blocklist.txt");
}

QString AppPaths::stateDir()
{
    return userDataDir() + QStringLiteral("/state");
}

QString AppPaths::bookmarksPath()
{
    return stateDir() + QStringLiteral("/bookmarks.json");
}

QString AppPaths::historyPath()
{
    return stateDir() + QStringLiteral("/history.json");
}

QString AppPaths::pinnedSitesPath()
{
    return stateDir() + QStringLiteral("/pinned-sites.json");
}

QString AppPaths::legacyHomeLayoutPath()
{
    return stateDir() + QStringLiteral("/home.json");
}

QString AppPaths::siteStoragePurgeMarker()
{
    return userDataDir() + QStringLiteral("/purge-site-storage.request");
}

QString AppPaths::appDir()
{
    return QCoreApplication::applicationDirPath();
}

void AppPaths::ensureCreated()
{
    QDir().mkpath(userDataDir());
    QDir().mkpath(profileDir());
    QDir().mkpath(logsDir());
    QDir().mkpath(privacyDir());
    QDir().mkpath(stateDir());
}

void AppPaths::ensureResourcesLoaded()
{
    // Q_INIT_RESOURCE must be called from the global namespace (or a function
    // outside any namespace), and it is a macro that expands to a local static
    // initialiser guarded by a name, so a one-shot lambda keeps it safe to call
    // repeatedly.
    static const bool once = [] {
        Q_INIT_RESOURCE(resources);
        return true;
    }();
    Q_UNUSED(once)
}

}  // namespace yozora
