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

QString AppPaths::appDir()
{
    return QCoreApplication::applicationDirPath();
}

void AppPaths::ensureCreated()
{
    QDir().mkpath(userDataDir());
    QDir().mkpath(profileDir());
    QDir().mkpath(logsDir());
}

}  // namespace yozora
