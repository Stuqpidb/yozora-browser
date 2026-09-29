// SPDX-License-Identifier: MIT
#include "privacy/TrackerList.h"

#include "app/AppPaths.h"

#include <QFile>
#include <QIODevice>
#include <QRegularExpression>

namespace yozora {

namespace {
// Reads a UTF-8 text file into lines. Returns nothing when the file is missing.
QStringList readLines(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
}
}  // namespace

QString TrackerList::domainFromRule(const QString& line)
{
    QString text = line.trimmed();
    if (text.isEmpty() || text.startsWith(QLatin1Char('#'))
        || text.startsWith(QLatin1Char('!'))) {
        return {};
    }

    // Inline comments are common in hand written lists.
    const int hash = text.indexOf(QLatin1Char('#'));
    if (hash >= 0) {
        text = text.left(hash).trimmed();
    }
    if (text.isEmpty()) {
        return {};
    }

    // A hosts-file line ("0.0.0.0 tracker.example" / "127.0.0.1 tracker.example")
    // is a very common way to paste a blocklist, so accept it too.
    if (text.startsWith(QLatin1String("0.0.0.0"))
        || text.startsWith(QLatin1String("127.0.0.1"))) {
        const QStringList parts = text.split(QRegularExpression(QStringLiteral("\\s+")),
                                             Qt::SkipEmptyParts);
        text = parts.size() >= 2 ? parts.at(1) : QString();
    }

    if (text.isEmpty()) {
        return {};
    }

    // Adblock-style domain anchor "||domain^".
    if (text.startsWith(QLatin1String("||"))) {
        text.remove(0, 2);
    }

    // Anything with a path, wildcard or option is not a plain domain rule and
    // is deliberately ignored instead of guessed at.
    static const QString unsupported = QStringLiteral("/*?$|\\");
    for (const QChar ch : unsupported) {
        if (text.contains(ch)) {
            return {};
        }
    }
    if (text.endsWith(QLatin1Char('^'))) {
        text.chop(1);
    }

    text = text.trimmed().toLower();
    while (text.startsWith(QLatin1Char('.'))) {
        text.remove(0, 1);
    }

    // A real rule must look like a dotted host name with no spaces.
    if (text.isEmpty() || !text.contains(QLatin1Char('.')) || text.contains(QLatin1Char(' '))) {
        return {};
    }
    return text;
}

void TrackerList::addLines(const QStringList& lines)
{
    for (const QString& line : lines) {
        const QString domain = domainFromRule(line);
        if (!domain.isEmpty()) {
            m_domains.insert(domain);
        }
    }
}

TrackerList TrackerList::fromLines(const QStringList& lines)
{
    TrackerList list;
    list.addLines(lines);
    return list;
}

TrackerList TrackerList::load()
{
    TrackerList list;
    list.addLines(readLines(QStringLiteral(":/privacy/trackers.txt")));
    // Local additions are optional and benefit only this user.
    list.addLines(readLines(AppPaths::trackerListPath()));
    return list;
}

bool TrackerList::isBlocked(const QString& host) const
{
    if (host.isEmpty() || m_domains.isEmpty()) {
        return false;
    }
    QString candidate = host.toLower();
    if (candidate.endsWith(QLatin1Char('.'))) {
        candidate.chop(1);
    }
    // Walk up the domain labels: "ads.tracker.example.com" is also matched by a
    // rule for "tracker.example.com".
    while (!candidate.isEmpty()) {
        if (m_domains.contains(candidate)) {
            return true;
        }
        const int dot = candidate.indexOf(QLatin1Char('.'));
        if (dot < 0) {
            break;
        }
        candidate = candidate.mid(dot + 1);
    }
    return false;
}

}  // namespace yozora
