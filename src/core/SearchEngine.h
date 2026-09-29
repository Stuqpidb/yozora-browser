// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QString>

namespace yozora {

// A single search provider. Keeping this a value type makes it trivial to
// extend later (custom engines, locale specific defaults) without touching
// the address bar.
struct SearchEngine {
    QString id;
    QString name;
    QString queryUrl;  // contains a %1 placeholder for the encoded query
    QString suggestUrl;  // optional, may be empty

    // Builds the final URL for `query` (already percent encoded by caller).
    [[nodiscard]] QString urlForQuery(const QString& query) const;
};

class SearchEngines {
public:
    [[nodiscard]] static QList<SearchEngine> builtin();
    [[nodiscard]] static SearchEngine byId(const QString& id);
    [[nodiscard]] static QString defaultId();

    // DuckDuckGo is the default: it does not require an API key and does not
    // track the user across sites.
    static constexpr auto kDuckDuckGoId = "duckduckgo";
    static constexpr auto kGoogleId = "google";
    static constexpr auto kBingId = "bing";
    static constexpr auto kBraveId = "brave";
    static constexpr auto kYandexId = "yandex";
};

}  // namespace yozora
