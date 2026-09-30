// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class QUrl;

namespace yozora {

// Builds Yozora's own error page.
//
// Chromium's built-in error page is turned off
// (QWebEngineSettings::ErrorPageEnabled = false), so every failure is reported
// through QWebEnginePage::loadingChanged() and rendered here instead. That
// keeps a Yozora look instead of a Chrome look-alike, and gives us a place to
// explain what actually went wrong.
class ErrorPage {
public:
    // Chromium net::Error values we care about. Kept as named constants
    // because the numeric codes are otherwise unreadable.
    enum NetError {
        Failed = -2,
        Aborted = -3,
        FileNotFound = -6,
        TimedOut = -7,
        NameNotResolved = -9,
        InternetDisconnected = -10,
        ConnectionClosed = -100,
        ConnectionReset = -101,
        ConnectionRefused = -102,
        ConnectionAborted = -103,
        ConnectionFailed = -104,
        NameResolutionFailed = -105,
        SslProtocolError = -107,
        AddressUnreachable = -109,
        ConnectionTimedOut = -118,
        // Certificate errors (net::ERR_CERT_*). Yozora never lets these through;
        // naming them lets the page explain what went wrong.
        SslPinnedKeyNotInCertificateChain = -150,
        CertificateCommonNameInvalid = -200,
        CertificateDateInvalid = -201,
        CertificateAuthorityInvalid = -202,
        CertificateContainsErrors = -203,
        CertificateRevoked = -206,
        CertificateInvalid = -207,
        CertificateWeakSignatureAlgorithm = -208,
        CertificateWeakKey = -211,
        CertificateTransparencyRequired = -214,
        CertificateKnownInterceptionBlocked = -217,
        TooManyRedirects = -310,
        InvalidUrl = -300,
        DisallowedUrlScheme = -301,
        UnknownUrlScheme = -302,
        UnsafePort = -312,
        InsecureResponse = -501,
        BlockedByClient = -20,
        BlockedByOrb = -19,
        CacheMiss = -400,
    };

    enum Domain {
        DomainNone = 0,
        DomainInternal = 1,
        DomainConnection = 2,
        DomainCertificate = 3,
        DomainHttp = 4,
        DomainDns = 7,
    };

    // Short headline, e.g. "Site not found".
    [[nodiscard]] static QString titleFor(int domain, int errorCode);

    // One or two sentences explaining what happened and what to try next.
    [[nodiscard]] static QString hintFor(int domain, int errorCode);

    // Complete offline HTML document, ready for QWebEnginePage::setHtml().
    [[nodiscard]] static QString html(const QUrl& url, int domain, int errorCode,
                                      const QString& errorString);

    // Convenience overload for "this host does not resolve".
    [[nodiscard]] static QString htmlForHost(const QString& host, const QString& reason);
};

}  // namespace yozora
