// SPDX-License-Identifier: MIT
#include "core/SearchEngine.h"

#include <QUrl>

namespace yozora {

QString SearchEngine::urlForQuery(const QString& query) const
{
    if (queryUrl.isEmpty()) {
        return {};
    }
    // queryUrl carries a %1 placeholder; %1 is the lowest legal marker
    // QString::arg() understands, so a search query that itself contains
    // "%1" cannot be substituted twice.
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

QString SearchEngines::defaultId()
{
    return QLatin1String(kDuckDuckGoId);
}

}  // namespace yozora
