// SPDX-License-Identifier: MIT
#include "privacy/DownloadSafety.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>

namespace yozora::downloads {

namespace {

// Names Windows reserves for devices. A file called "NUL" or "COM1" cannot be
// created normally and can be abused to redirect writes, so they are defused.
bool isReservedWindowsName(const QString& baseName)
{
    static const QSet<QString> reserved = {
        QStringLiteral("con"),  QStringLiteral("prn"),  QStringLiteral("aux"),
        QStringLiteral("nul"),  QStringLiteral("com1"), QStringLiteral("com2"),
        QStringLiteral("com3"), QStringLiteral("com4"), QStringLiteral("com5"),
        QStringLiteral("com6"), QStringLiteral("com7"), QStringLiteral("com8"),
        QStringLiteral("com9"), QStringLiteral("lpt1"), QStringLiteral("lpt2"),
        QStringLiteral("lpt3"), QStringLiteral("lpt4"), QStringLiteral("lpt5"),
        QStringLiteral("lpt6"), QStringLiteral("lpt7"), QStringLiteral("lpt8"),
        QStringLiteral("lpt9"),
    };
    return reserved.contains(baseName.toLower());
}

// Extensions whose files can run code, install software or launch the shell.
bool hasDangerousExtension(const QString& suffix)
{
    static const QSet<QString> dangerous = {
        QStringLiteral("exe"), QStringLiteral("msi"), QStringLiteral("msp"),
        QStringLiteral("bat"), QStringLiteral("cmd"), QStringLiteral("com"),
        QStringLiteral("scr"), QStringLiteral("pif"), QStringLiteral("ps1"),
        QStringLiteral("psm1"), QStringLiteral("vbs"), QStringLiteral("vbe"),
        QStringLiteral("js"), QStringLiteral("jse"), QStringLiteral("wsf"),
        QStringLiteral("wsh"), QStringLiteral("hta"), QStringLiteral("jar"),
        QStringLiteral("reg"), QStringLiteral("lnk"), QStringLiteral("msix"),
        QStringLiteral("appx"), QStringLiteral("dll"), QStringLiteral("cpl"),
        QStringLiteral("gadget"), QStringLiteral("inf"), QStringLiteral("ins"),
        QStringLiteral("isp"), QStringLiteral("msc"), QStringLiteral("sct"),
        QStringLiteral("shb"), QStringLiteral("sys"),
#if !defined(Q_OS_WIN)
        QStringLiteral("sh"), QStringLiteral("run"), QStringLiteral("desktop"),
        QStringLiteral("appimage"),
#endif
    };
    return dangerous.contains(suffix.toLower());
}

}  // namespace

QString sanitizeFileName(const QString& proposed)
{
    // Treat both separators as separators regardless of platform: a Windows
    // style name must not smuggle a path through on Linux and vice versa.
    QString name = proposed;
    name.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const int slash = name.lastIndexOf(QLatin1Char('/'));
    if (slash >= 0) {
        name = name.mid(slash + 1);
    }

    // Drop control characters and the characters Windows forbids in a name.
    QString cleaned;
    cleaned.reserve(name.size());
    for (const QChar ch : name) {
        const ushort code = ch.unicode();
        if (code < 0x20 || code == 0x7F) {
            continue;
        }
        if (QStringLiteral("<>:\"|?*").contains(ch)) {
            cleaned.append(QLatin1Char('_'));
        } else {
            cleaned.append(ch);
        }
    }
    name = cleaned;

    // No leading dots (would create hidden files) and no trailing dots or
    // spaces (forbidden by NTFS and used to defeat extension checks).
    while (name.startsWith(QLatin1Char('.'))) {
        name.remove(0, 1);
    }
    while (name.endsWith(QLatin1Char('.')) || name.endsWith(QLatin1Char(' '))) {
        name.chop(1);
    }

    if (name.isEmpty()) {
        return QStringLiteral("download");
    }

    // Defuse Windows device names, keeping the extension intact.
    const QString baseName = name.section(QLatin1Char('.'), 0, 0);
    if (isReservedWindowsName(baseName)) {
        name.prepend(QLatin1Char('_'));
    }

    // Keep the name to a sane length without cutting the extension off.
    constexpr int kMaxLength = 200;
    if (name.size() > kMaxLength) {
        const QFileInfo info(name);
        const QString suffix = info.suffix();
        const int keep = kMaxLength - (suffix.isEmpty() ? 0 : suffix.size() + 1);
        name = name.left(qMax(1, keep));
        if (!suffix.isEmpty()) {
            name += QLatin1Char('.') + suffix;
        }
    }

    return name;
}

bool isDangerousFile(const QString& fileName)
{
    const QFileInfo info(fileName);
    const QString suffix = info.suffix();
    return !suffix.isEmpty() && hasDangerousExtension(suffix);
}

QString uniquePath(const QString& directory, const QString& fileName)
{
    const QDir dir(directory);
    const QString first = dir.filePath(fileName);
    if (!QFileInfo::exists(first)) {
        return first;
    }

    const QFileInfo info(fileName);
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix();
    const auto compose = [&](const QString& stem) {
        QString candidate = stem;
        if (!suffix.isEmpty()) {
            candidate += QLatin1Char('.') + suffix;
        }
        return dir.filePath(candidate);
    };

    for (int i = 1; i < 10000; ++i) {
        const QString candidate = compose(QStringLiteral("%1 (%2)").arg(base).arg(i));
        if (!QFileInfo::exists(candidate)) {
            return candidate;
        }
    }
    return compose(base + QStringLiteral(" (copy)"));
}

}  // namespace yozora::downloads
