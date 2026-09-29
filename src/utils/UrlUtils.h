// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

namespace yozora {

// URL normalisation and "did the user type a search or a URL?" detection.
//
// This header is intentionally free of any Qt WebEngine dependency so that
// the logic can be unit tested in isolation and reused by other front-ends.
namespace url {

// Result of classifying raw text typed into the address bar.
enum class InputKind {
    Url,    // Already a valid absolute URL.
    Search, // Free-form text, must be turned into a search query.
    Empty,  // Nothing but whitespace.
};

// Adds a scheme when the input clearly looks like a host ("example.com",
// "example.com/path", "localhost:8080"). Anything that cannot be a host is
// returned untouched so the search engine can handle it.
QString withScheme(const QString& input);

// Full normalisation used before loading: trims, adds a scheme when needed
// and rejects obviously malformed input (returns an empty string).
QString normalize(const QString& input);

// Classifies address-bar input without converting it to a URL.
InputKind classify(const QString& input);

// True when `text` is a syntactically valid absolute URL.
bool isUrl(const QString& text);

// Escapes a plain string so it is safe to embed into a search engine URL.
QString toSearchQuery(const QString& text);

// Percent-decoded, human readable form of a URL, used for display purposes.
QString toDisplayString(const QString& url);

// Returns the URL of the directory containing the resources of a page,
// used to resolve favicons. Returns an empty string when the page has no
// meaningful origin (for example "about:blank").
QString baseUrlFor(const QString& pageUrl);

}  // namespace url
}  // namespace yozora
