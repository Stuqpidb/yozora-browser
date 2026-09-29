// SPDX-License-Identifier: MIT
#pragma once

#include <QSet>
#include <QString>
#include <QStringList>

namespace yozora {

// A compiled set of tracker domains.
//
// The list is built once (from the bundled resource and an optional local file)
// and then treated as immutable. That matters because matching happens on the
// Chromium IO thread, where taking a lock on every request would be wasteful:
// instead, RequestInterceptor swaps whole TrackerList instances atomically.
class TrackerList {
public:
    // Builds the list from the bundled resource and, when it exists, from a
    // local "blocklist.txt" file next to the profile. The local file is never
    // uploaded anywhere; it only lets a user extend the built-in list.
    [[nodiscard]] static TrackerList load();

    // Builds a list from raw lines. Exposed for tests.
    [[nodiscard]] static TrackerList fromLines(const QStringList& lines);

    [[nodiscard]] bool isEmpty() const { return m_domains.isEmpty(); }
    [[nodiscard]] int size() const { return static_cast<int>(m_domains.size()); }

    // True when `host` or one of its parent domains is a known tracker.
    [[nodiscard]] bool isBlocked(const QString& host) const;

    // Extracts a bare domain from a single filter-list line. Returns an empty
    // string for comments, blanks and any rule syntax this simple matcher does
    // not support, so unsupported rules are skipped rather than misapplied.
    [[nodiscard]] static QString domainFromRule(const QString& line);

private:
    void addLines(const QStringList& lines);

    QSet<QString> m_domains;
};

}  // namespace yozora
