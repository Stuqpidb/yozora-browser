// SPDX-License-Identifier: MIT
#include "utils/UrlUtils.h"

#include <QRegularExpression>
#include <QUrl>

namespace yozora::url {

namespace {

// A hostname label: alphanumeric, dashes and underscores, not starting or
// ending with a dash. Requiring at least one dot keeps "hello" a search but
// makes "example.com" a URL.
const QRegularExpression& kHostPattern()
{
    static const QRegularExpression re(
        QStringLiteral(
            "^(?=.{1,253}$)"
            "(?:[a-z0-9](?:[a-z0-9-]{0,61}[a-z0-9])?\\.)+"
            "[a-z]{2,63}$"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

const QRegularExpression& kLocalHostPattern()
{
    static const QRegularExpression re(
        QStringLiteral("^(localhost|127\\.0\\.0\\.1|\\[?::1\\]?)(:\\d{1,5})?$"),
        QRegularExpression::CaseInsensitiveOption);
    return re;
}

const QRegularExpression& kIpv4Pattern()
{
    static const QRegularExpression re(
        QStringLiteral("^\\d{1,3}(?:\\.\\d{1,3}){3}(:\\d{1,5})?$"));
    return re;
}

bool hasNonAscii(const QString& text)
{
    for (const QChar ch : text) {
        if (ch.unicode() > 0x7F) {
            return true;
        }
    }
    return false;
}

bool isAllDigits(const QString& text)
{
    if (text.isEmpty()) {
        return false;
    }
    for (const QChar ch : text) {
        if (!ch.isDigit()) {
            return false;
        }
    }
    return true;
}

// True when the text really carries a scheme, as opposed to a bare host
// followed by a port: "localhost:8080" is a host and a port, not a scheme.
bool hasRealScheme(const QString& text)
{
    static const QRegularExpression schemeRe(QStringLiteral("^[a-zA-Z][a-zA-Z0-9+.-]*:"));
    const auto match = schemeRe.match(text);
    if (!match.hasMatch()) {
        return false;
    }
    // Everything after "scheme:" must not be a bare port number.
    return !isAllDigits(text.mid(match.capturedLength()));
}

}  // namespace

QString withScheme(const QString& input)
{
    const QString text = input.trimmed();
    if (text.isEmpty()) {
        return {};
    }

    // Already carries a scheme? (http:, https:, file:, about:, data:, ...)
    if (hasRealScheme(text)) {
        return text;
    }

    // "//example.com" - protocol relative URL, treat as https.
    if (text.startsWith(QLatin1String("//"))) {
        return QStringLiteral("https:") + text;
    }

    // Bare host, host with port, or a host with a path/query.
    QString candidate = text;
    if (candidate.startsWith(QLatin1Char('/'))) {
        // A bare path is not a URL; leave it for the search engine.
        return text;
    }

    // Split off a trailing path so "example.com/foo" still matches the host
    // pattern, while "example.com" + " " + "text" does not.
    QString hostPart = candidate;
    QString rest;
    const int slash = candidate.indexOf(QLatin1Char('/'));
    if (slash >= 0) {
        hostPart = candidate.left(slash);
        rest = candidate.mid(slash);
    } else {
        const int q = candidate.indexOf(QLatin1Char('?'));
        if (q >= 0) {
            hostPart = candidate.left(q);
            rest = candidate.mid(q);
        }
    }

    // A port only counts when it directly follows the host.
    QString hostOnly = hostPart;
    const int colon = hostPart.lastIndexOf(QLatin1Char(':'));
    if (colon > 0 && !hostPart.startsWith(QLatin1Char('['))) {
        const QString maybePort = hostPart.mid(colon + 1);
        bool numeric = !maybePort.isEmpty();
        for (const QChar ch : maybePort) {
            if (!ch.isDigit()) {
                numeric = false;
                break;
            }
        }
        if (numeric) {
            hostOnly = hostPart.left(colon);
        }
    }

    if (kHostPattern().match(hostOnly).hasMatch() || kIpv4Pattern().match(hostOnly).hasMatch()
        || kLocalHostPattern().match(hostOnly).hasMatch()
        || kLocalHostPattern().match(hostPart).hasMatch()) {
        return QStringLiteral("https://") + candidate;
    }

    return candidate;
}

bool isUrl(const QString& text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }
    const QUrl parsed(trimmed, QUrl::StrictMode);
    if (!parsed.isValid() || parsed.scheme().length() < 2) {
        return false;
    }
    if (!parsed.host().isEmpty()) {
        return true;
    }
    // Schemes without an authority component are still perfectly valid URLs.
    static const QStringList schemeless = {
        QStringLiteral("about"),  QStringLiteral("data"),  QStringLiteral("mailto"),
        QStringLiteral("tel"),    QStringLiteral("sms"),   QStringLiteral("javascript"),
        QStringLiteral("magnet"), QStringLiteral("blob"),  QStringLiteral("ws"),
        QStringLiteral("wss"),    QStringLiteral("ftp"),   QStringLiteral("file"),
    };
    return schemeless.contains(parsed.scheme(), Qt::CaseInsensitive);
}

QString normalize(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const QString candidate = withScheme(trimmed);
    if (!isUrl(candidate)) {
        return {};
    }

    QUrl parsed = QUrl::fromUserInput(candidate);
    if (!parsed.isValid()) {
        return {};
    }
    if (parsed.scheme().isEmpty()) {
        return {};
    }

    // Default to HTTPS when no scheme was typed.
    if (parsed.scheme().compare(QLatin1String("http"), Qt::CaseInsensitive) == 0
        || parsed.scheme().compare(QLatin1String("https"), Qt::CaseInsensitive) == 0) {
        if (parsed.host().isEmpty()) {
            return {};
        }
    }

    return parsed.toString();
}

InputKind classify(const QString& input)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return InputKind::Empty;
    }

    if (hasNonAscii(trimmed)) {
        return InputKind::Search;
    }

    // An explicit scheme always wins.
    if (hasRealScheme(trimmed)) {
        return isUrl(trimmed) ? InputKind::Url : InputKind::Search;
    }

    const QString candidate = withScheme(trimmed);
    return isUrl(candidate) ? InputKind::Url : InputKind::Search;
}

QString toSearchQuery(const QString& text)
{
    return QString::fromUtf8(QUrl::toPercentEncoding(text.trimmed()));
}

QString toDisplayString(const QString& url)
{
    const QUrl parsed(url);
    if (!parsed.isValid()) {
        return url;
    }
    if (parsed.scheme() == QLatin1String("about") || parsed.scheme() == QLatin1String("qrc")) {
        return url;
    }

    // toDisplayString() drops the trailing slash only for the root path, so
    // "https://example.com/" still arrives as "example.com/" once the scheme
    // has been removed. Strip it here instead of leaking a trailing slash into
    // the address bar.
    QString display = parsed.toDisplayString(QUrl::StripTrailingSlash);
    if (display.startsWith(QLatin1String("https://"))) {
        display.remove(0, 8);
    } else if (display.startsWith(QLatin1String("http://"))) {
        display.remove(0, 7);
    }
    if (display.endsWith(QLatin1Char('/'))) {
        display.chop(1);
    }

    if (display.startsWith(QLatin1String("www."))) {
        display.remove(0, 4);
    }
    return display;
}

QString baseUrlFor(const QString& pageUrl)
{
    const QUrl parsed(pageUrl);
    if (!parsed.isValid() || parsed.scheme().isEmpty()) {
        return {};
    }
    if (parsed.scheme() == QLatin1String("about") || parsed.scheme() == QLatin1String("qrc")
        || parsed.scheme() == QLatin1String("data")) {
        return {};
    }
    return parsed.toString();
}

}  // namespace yozora::url
