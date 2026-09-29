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

    // True when `queryUrl` can be used as a custom engine: it must be an
    // absolute HTTPS/HTTP URL carrying a single "%s" placeholder. The "%s"
    // marker (rather than "%1") keeps the templating obvious to the user, who
    // types this value in by hand.
    [[nodiscard]] static bool isValidCustomUrl(const QString& queryUrl);

    // Builds a custom engine from user supplied values. Returns an engine with
    // an empty queryUrl when the URL is not usable.
    [[nodiscard]] static SearchEngine custom(const QString& name, const QString& queryUrl);

    // DuckDuckGo is the default: it does not require an API key and does not
    // track the user across sites.
    static constexpr auto kDuckDuckGoId = "duckduckgo";
    static constexpr auto kGoogleId = "google";
    static constexpr auto kBingId = "bing";
    static constexpr auto kBraveId = "brave";
    static constexpr auto kYandexId = "yandex";
    static constexpr auto kCustomId = "custom";
};

}  // namespace yozora
