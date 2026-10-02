// SPDX-License-Identifier: MIT
#include "privacy/FilterEngine.h"

#include "app/AppPaths.h"

#include <QFile>
#include <QIODevice>
#include <QRegularExpression>

namespace yozora {

namespace {

QStringList readLines(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }
    return QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
}

ResourceType typeFromName(const QString& name)
{
    // The common adblock resource-type options, mapped onto our enum.
    if (name == QLatin1String("script")) {
        return ResourceType::Script;
    }
    if (name == QLatin1String("image")) {
        return ResourceType::Image;
    }
    if (name == QLatin1String("stylesheet") || name == QLatin1String("css")) {
        return ResourceType::Stylesheet;
    }
    if (name == QLatin1String("font")) {
        return ResourceType::Font;
    }
    if (name == QLatin1String("media")) {
        return ResourceType::Media;
    }
    if (name == QLatin1String("xmlhttprequest") || name == QLatin1String("xhr")) {
        return ResourceType::XmlHttpRequest;
    }
    if (name == QLatin1String("subdocument") || name == QLatin1String("frame")) {
        return ResourceType::Subdocument;
    }
    if (name == QLatin1String("ping")) {
        return ResourceType::Ping;
    }
    if (name == QLatin1String("websocket")) {
        return ResourceType::Websocket;
    }
    return ResourceType::Other;
}

// Walks up a host to its registrable-ish parent domains and reports whether
// `host` equals `domain` or is a subdomain of it. This is the domain anchor
// "||domain^" semantics without a public-suffix list: close enough for blocking
// and impossible to get dangerously wrong.
bool hostWithinDomain(const QString& host, const QString& domain)
{
    if (host == domain) {
        return true;
    }
    return host.endsWith(QLatin1Char('.') + domain);
}

QString normalizedHost(QString host)
{
    host = host.toLower();
    while (host.endsWith(QLatin1Char('.'))) {
        host.chop(1);
    }
    return host;
}

}  // namespace

bool FilterEngine::parseRule(const QString& line, const QString& category, FilterRule* out)
{
    QString text = line.trimmed();
    if (text.isEmpty() || text.startsWith(QLatin1Char('#')) || text.startsWith(QLatin1Char('!'))) {
        return false;
    }

    // Strip a trailing comment that is not part of a $ option list.
    const int hash = text.indexOf(QLatin1Char('#'));
    if (hash > 0) {
        text = text.left(hash).trimmed();
    }
    if (text.isEmpty()) {
        return false;
    }

    FilterRule rule;
    rule.value.clear();

    // Split off the "$type,type,third-party" option tail. The dollar is not part
    // of the pattern itself.
    QStringList options;
    const int dollar = text.lastIndexOf(QLatin1Char('$'));
    if (dollar >= 0) {
        options = text.mid(dollar + 1).split(QLatin1Char(','), Qt::SkipEmptyParts);
        text = text.left(dollar).trimmed();
    }

    for (const QString& optionRaw : options) {
        QString option = optionRaw.trimmed().toLower();
        bool negated = false;
        if (option.startsWith(QLatin1Char('~'))) {
            negated = true;
            option.remove(0, 1);
        }
        if (option == QLatin1String("third-party")) {
            rule.matchThirdPartyOnly = !negated;
        } else if (option == QLatin1String("first-party")) {
            rule.matchFirstPartyOnly = !negated;
        } else if (negated) {
            // "~image" etc: an exclusion. The simple engine does not support
            // per-type exclusions, so a rule that uses one is skipped rather than
            // applied more broadly than written.
            return false;
        } else {
            rule.types.insert(static_cast<int>(typeFromName(option)));
        }
    }

    // The adblock domain anchor: ||domain^ (an optional trailing ^ and path).
    if (text.startsWith(QLatin1String("||"))) {
        QString body = text.mid(2);
        // Keep the host portion; a path after the anchor makes the rule more
        // specific than our domain matcher can honour, so it is skipped.
        const int slash = body.indexOf(QLatin1Char('/'));
        if (slash >= 0) {
            return false;
        }
        body.remove(QLatin1Char('^'));
        body = normalizedHost(body);
        if (body.isEmpty() || !body.contains(QLatin1Char('.'))) {
            return false;
        }
        rule.kind = FilterRule::Kind::Domain;
        rule.value = body;
        *out = rule;
        return true;
    }

    // A plain host line pasted from a hosts file or a simple list.
    if (text.startsWith(QLatin1String("0.0.0.0")) || text.startsWith(QLatin1String("127.0.0.1"))) {
        const QStringList parts =
            text.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            return false;
        }
        text = parts.at(1);
    }

    // A bare domain with no path, wildcard or placeholder is treated as a domain
    // rule. This is what hand-written tracker lists look like.
    if (text.contains(QLatin1Char('.')) && !text.contains(QLatin1Char('/'))
        && !text.contains(QLatin1Char('*')) && !text.contains(QLatin1Char('?'))
        && !text.contains(QLatin1Char(' ')) && !text.contains(QLatin1Char('|'))
        && !text.contains(QLatin1Char('\\'))) {
        QString body = text;
        if (body.startsWith(QLatin1Char('.'))) {
            body.remove(0, 1);
        }
        body = normalizedHost(body);
        if (body.isEmpty()) {
            return false;
        }
        rule.kind = FilterRule::Kind::Domain;
        rule.value = body;
        *out = rule;
        return true;
    }

    // A path/substring rule: block when the URL contains this text. This is what
    // catches "/ads/", "/pagead/", "banner.", and so on.
    if (text.contains(QLatin1Char('/')) || text.contains(QLatin1Char('*'))) {
        // A leading wildcard on an otherwise bare host ("*.tracker.example") is
        // a domain rule in disguise, but our simple matcher does not model it;
        // skipping it is safer than turning it into a substring.
        if (text.startsWith(QLatin1Char('*')) && !text.contains(QLatin1Char('/'))) {
            return false;
        }
        QString pattern = text;
        pattern.remove(QLatin1Char('*'));  // wildcards degrade to a substring
        if (pattern.startsWith(QLatin1Char('|'))) {
            pattern.remove(0, 1);
        }
        if (pattern.endsWith(QLatin1Char('|'))) {
            pattern.chop(1);
        }
        if (pattern.endsWith(QLatin1Char('^'))) {
            pattern.chop(1);
        }
        if (pattern.length() < 4) {
            // Too short a substring blocks far too much to be safe.
            return false;
        }
        rule.kind = FilterRule::Kind::Substring;
        rule.value = pattern.toLower();
        *out = rule;
        return true;
    }

    Q_UNUSED(category)
    Q_UNUSED(out)
    return false;
}

FilterEngine FilterEngine::fromLines(const QStringList& adLines, const QStringList& trackerLines)
{
    FilterEngine engine;
    const auto add = [&engine](const QStringList& lines, bool isAd) {
        for (const QString& line : lines) {
            FilterRule rule;
            if (!parseRule(line, isAd ? QStringLiteral("ad") : QStringLiteral("tracker"), &rule)) {
                continue;
            }
            rule.isAd = isAd;
            engine.m_rules.append(rule);
        }
    };
    add(adLines, true);
    add(trackerLines, false);
    return engine;
}

FilterEngine FilterEngine::load()
{
    // The compiled lists live in the resource bundle; make sure it is registered
    // before reading from it.
    AppPaths::ensureResourcesLoaded();
    // Three local sources, all optional except the first two:
    //   :/privacy/ads.txt       - bundled advert rules
    //   :/privacy/trackers.txt  - bundled tracker rules
    //   privacy/blocklist.txt   - the user's own additions
    const QStringList userLines = readLines(AppPaths::trackerListPath());
    FilterEngine engine = fromLines(readLines(QStringLiteral(":/privacy/ads.txt")),
                                    readLines(QStringLiteral(":/privacy/trackers.txt")));
    // User rules are parsed the same way and treated as tracker rules, since
    // that is the more conservative label for the shield's counter.
    for (const QString& line : userLines) {
        FilterRule rule;
        if (parseRule(line, QStringLiteral("tracker"), &rule)) {
            rule.isAd = false;
            engine.m_rules.append(rule);
        }
    }
    return engine;
}

bool FilterEngine::matches(const FilterRule& rule, const QString& url, const QString& host,
                           ResourceType type, bool thirdParty) const
{
    if (!rule.hasType(type)) {
        return false;
    }
    if (rule.matchThirdPartyOnly && !thirdParty) {
        return false;
    }
    if (rule.matchFirstPartyOnly && thirdParty) {
        return false;
    }

    switch (rule.kind) {
        case FilterRule::Kind::Domain:
            return !host.isEmpty() && hostWithinDomain(host, rule.value);
        case FilterRule::Kind::Substring:
            return url.contains(rule.value, Qt::CaseInsensitive);
    }
    return false;
}

bool FilterEngine::shouldBlock(const QString& url, const QString& host, ResourceType type,
                               bool thirdParty) const
{
    const QString normalized = normalizedHost(host);
    const QString lowerUrl = url.toLower();
    for (const FilterRule& rule : m_rules) {
        if (matches(rule, lowerUrl, normalized, type, thirdParty)) {
            return true;
        }
    }
    return false;
}

FilterEngine::Category FilterEngine::classify(const QString& url, const QString& host,
                                              ResourceType type, bool thirdParty) const
{
    const QString normalized = normalizedHost(host);
    const QString lowerUrl = url.toLower();
    for (const FilterRule& rule : m_rules) {
        if (matches(rule, lowerUrl, normalized, type, thirdParty)) {
            return rule.isAd ? Category::Ad : Category::Tracker;
        }
    }
    return Category::None;
}

}  // namespace yozora
