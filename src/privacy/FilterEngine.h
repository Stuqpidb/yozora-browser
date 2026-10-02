// SPDX-License-Identifier: MIT
#pragma once

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>

namespace yozora {

// Which kind of request a rule is about. This mirrors the useful subset of the
// adblock "resource type" options ($script, $image, ...). A rule without type
// options matches every type; Network means the default "match any request".
enum class ResourceType {
    Network,
    Script,
    Image,
    Stylesheet,
    Font,
    Media,
    XmlHttpRequest,
    Subdocument,   // frames
    Ping,
    Websocket,
    Other,
};

// One compiled filter rule. The struct is deliberately plain data so a list of
// them can be built on one thread and read without locking on another.
struct FilterRule {
    enum class Kind {
        Domain,       // blocks a domain and every subdomain of it
        Substring,    // blocks when the URL contains a substring
    };

    Kind kind = Kind::Domain;
    QString value;                 // the domain, or the substring to search for
    QSet<int> types;               // empty means "every request type"
    bool matchThirdPartyOnly = false;
    bool matchFirstPartyOnly = false;
    bool isAd = false;             // false: tracker rule

    [[nodiscard]] bool hasType(ResourceType type) const
    {
        return types.isEmpty() || types.contains(static_cast<int>(type));
    }
};

// A compiled ad/tracker filter list.
//
// This replaces the old "list of tracker domains" with a small but real
// adblock-style engine, in the spirit of Brave: rules can carry resource-type
// and party options, and matching classifies a blocked request so the UI can say
// whether it was an advert or a tracker.
//
// The list is built once (from the bundled resources and an optional local file)
// and then treated as immutable. Matching runs on the Chromium IO thread, where
// taking a lock per request would be wasteful, so RequestInterceptor swaps whole
// FilterEngine instances atomically instead.
class FilterEngine {
public:
    enum class Category {
        None,
        Ad,
        Tracker,
    };

    [[nodiscard]] static FilterEngine load();

    [[nodiscard]] static FilterEngine fromLines(const QStringList& adLines,
                                                const QStringList& trackerLines);

    // Parses one adblock-style line into a rule. Returns false for comments,
    // blanks and syntax this engine does not support, so unsupported rules are
    // skipped rather than misapplied.
    [[nodiscard]] static bool parseRule(const QString& line, const QString& category,
                                        FilterRule* out);

    [[nodiscard]] bool isEmpty() const { return m_rules.isEmpty(); }
    [[nodiscard]] int size() const { return static_cast<int>(m_rules.size()); }

    // True when the request should be blocked. Matches the rule set against the
    // request URL, its type and whether it is third party to the page.
    [[nodiscard]] bool shouldBlock(const QString& url, const QString& host, ResourceType type,
                                   bool thirdParty) const;

    // The category of the first matching rule, for the UI. Ad is checked first
    // so an advert served from a shared ad/tracker host is reported as an ad.
    [[nodiscard]] Category classify(const QString& url, const QString& host, ResourceType type,
                                    bool thirdParty) const;

private:
    [[nodiscard]] bool matches(const FilterRule& rule, const QString& url, const QString& host,
                               ResourceType type, bool thirdParty) const;

    QStringList m_adDomains;
    QStringList m_trackerDomains;
    QList<FilterRule> m_rules;
};

}  // namespace yozora
