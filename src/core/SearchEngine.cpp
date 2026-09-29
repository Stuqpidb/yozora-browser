// SPDX-License-Identifier: MIT
#include "core/SearchEngine.h"

#include <QUrl>

namespace yozora {

QString SearchEngine::urlForQuery(const QString& query) const
{
    if (queryUrl.isEmpty()) {
        return {};
    }
    // The user-facing custom template uses "%s"; the built-in engines use
    // "%1". The query is already percent encoded, so it can never contain a
    // literal placeholder and a plain replace is safe.
    if (queryUrl.contains(QLatin1String("%s"))) {
        QString url = queryUrl;
        url.replace(QLatin1String("%s"), query);
        return url;
    }
    return queryUrl.arg(query, QStringLiteral("%1"));
}

QList<SearchEngine> SearchEngines::builtin()
{
    return {
        {QLatin1String(kDuckDuckGoId),
         QStringLiteral("DuckDuckGo"),
         QStringLiteral("https://duckduckgo.com/?q=%1"),
         QStringLiteral("https://duckduckgo.com/ac/?q=%1&type=list")},
        {QLatin1String(kGoogleId), QStringLiteral("Google"),
         QStringLiteral("https://www.google.com/search?q=%1"),
         QStringLiteral("https://suggestqueries.google.com/complete/search?client=firefox&q=%1")},
        {QLatin1String(kBingId), QStringLiteral("Bing"),
         QStringLiteral("https://www.bing.com/search?q=%1"),
         QStringLiteral("https://api.bing.com/osjson.aspx?query=%1")},
        {QLatin1String(kBraveId), QStringLiteral("Brave"),
         QStringLiteral("https://search.brave.com/search?q=%1"), QString()},
        {QLatin1String(kYandexId), QStringLiteral("Yandex"),
         QStringLiteral("https://yandex.com/search/?text=%1"), QString()},
    };
}

SearchEngine SearchEngines::byId(const QString& id)
{
    const auto engines = builtin();
    for (const auto& engine : engines) {
        if (engine.id == id) {
            return engine;
        }
    }
    return byId(defaultId());
}

bool SearchEngines::isValidCustomUrl(const QString& queryUrl)
{
    const QString trimmed = queryUrl.trimmed();
    if (trimmed.isEmpty() || !trimmed.contains(QLatin1String("%s"))) {
        return false;
    }
    // Probe with the placeholder removed so "%s" is never mistaken for a
    // malformed percent escape.
    QString probe = trimmed;
    probe.replace(QLatin1String("%s"), QStringLiteral("yozora"));
    const QUrl url(probe, QUrl::StrictMode);
    if (!url.isValid() || url.host().isEmpty()) {
        return false;
    }
    const QString scheme = url.scheme().toLower();
    return scheme == QLatin1String("https") || scheme == QLatin1String("http");
}

SearchEngine SearchEngines::custom(const QString& name, const QString& queryUrl)
{
    SearchEngine engine;
    engine.id = QLatin1String(kCustomId);
    engine.name = name.trimmed().isEmpty() ? QStringLiteral("Custom") : name.trimmed();
    if (isValidCustomUrl(queryUrl)) {
        engine.queryUrl = queryUrl.trimmed();
    }
    return engine;
}

QString SearchEngines::defaultId()
{
    return QLatin1String(kDuckDuckGoId);
}

}  // namespace yozora
