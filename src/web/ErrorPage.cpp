// SPDX-License-Identifier: MIT
#include "web/ErrorPage.h"

#include "core/Theme.h"

#include <QUrl>

namespace yozora {

namespace {

QString escaped(const QString& text)
{
    return text.toHtmlEscaped();
}

QString titleForNet(int code)
{
    switch (code) {
        case ErrorPage::FileNotFound:
            return QStringLiteral("Page not found");
        case ErrorPage::NameNotResolved:
        case ErrorPage::NameResolutionFailed:
            return QStringLiteral("Site not found");
        case ErrorPage::ConnectionRefused:
            return QStringLiteral("Connection refused");
        case ErrorPage::ConnectionReset:
        case ErrorPage::ConnectionClosed:
        case ErrorPage::ConnectionAborted:
            return QStringLiteral("Connection closed");
        case ErrorPage::ConnectionFailed:
        case ErrorPage::AddressUnreachable:
            return QStringLiteral("Cannot reach the server");
        case ErrorPage::ConnectionTimedOut:
        case ErrorPage::TimedOut:
            return QStringLiteral("The site took too long to answer");
        case ErrorPage::InternetDisconnected:
            return QStringLiteral("You appear to be offline");
        case ErrorPage::SslProtocolError:
        case ErrorPage::InsecureResponse:
            return QStringLiteral("Secure connection failed");
        case ErrorPage::CertificateDateInvalid:
            return QStringLiteral("The site's certificate has expired");
        case ErrorPage::CertificateCommonNameInvalid:
            return QStringLiteral("The certificate does not match this site");
        case ErrorPage::CertificateAuthorityInvalid:
            return QStringLiteral("This site's certificate is not trusted");
        case ErrorPage::CertificateRevoked:
            return QStringLiteral("The site's certificate was revoked");
        case ErrorPage::SslPinnedKeyNotInCertificateChain:
            return QStringLiteral("This site's security key changed");
        case ErrorPage::TooManyRedirects:
            return QStringLiteral("Too many redirects");
        case ErrorPage::InvalidUrl:
        case ErrorPage::DisallowedUrlScheme:
        case ErrorPage::UnknownUrlScheme:
            return QStringLiteral("This address cannot be opened");
        case ErrorPage::UnsafePort:
            return QStringLiteral("This port is not allowed");
        case ErrorPage::BlockedByClient:
        case ErrorPage::BlockedByOrb:
            return QStringLiteral("Request blocked");
        case ErrorPage::CacheMiss:
            return QStringLiteral("Nothing to show here");
        case ErrorPage::Failed:
            return QStringLiteral("Cannot open the page");
        default:
            return QStringLiteral("Cannot open the page");
    }
}

QString hintForNet(int code)
{
    switch (code) {
        case ErrorPage::FileNotFound:
            return QStringLiteral("The server answered, but there is nothing at this address.");
        case ErrorPage::NameNotResolved:
        case ErrorPage::NameResolutionFailed:
            return QStringLiteral("Yozora could not find this site on the network. Check the "
                                 "address for a typo, or check your connection.");
        case ErrorPage::ConnectionRefused:
            return QStringLiteral("The site refused the connection. It may be offline, or its "
                                 "address or port may be wrong.");
        case ErrorPage::ConnectionReset:
        case ErrorPage::ConnectionClosed:
        case ErrorPage::ConnectionAborted:
            return QStringLiteral("The site closed the connection before the page finished "
                                 "loading.");
        case ErrorPage::ConnectionFailed:
        case ErrorPage::AddressUnreachable:
            return QStringLiteral("There was no route to this site. A firewall or proxy may be "
                                 "blocking it.");
        case ErrorPage::ConnectionTimedOut:
        case ErrorPage::TimedOut:
            return QStringLiteral("The site did not answer in time. It may be overloaded or your "
                                 "connection may be slow.");
        case ErrorPage::InternetDisconnected:
            return QStringLiteral("Yozora cannot reach the network. Reconnect and try again.");
        case ErrorPage::SslProtocolError:
        case ErrorPage::InsecureResponse:
            return QStringLiteral("Yozora could not verify the site's certificate. The "
                                 "connection is not protected, so loading stopped instead of "
                                 "continuing without checking.");
        case ErrorPage::CertificateDateInvalid:
            return QStringLiteral("The certificate has expired, so the connection cannot be "
                                 "trusted. The site's clock or its certificate is out of date.");
        case ErrorPage::CertificateCommonNameInvalid:
            return QStringLiteral("The certificate was issued for a different address. This can "
                                 "mean the connection is being intercepted by someone else.");
        case ErrorPage::CertificateAuthorityInvalid:
            return QStringLiteral("Yozora does not trust the authority that signed this "
                                 "certificate, so the connection was refused.");
        case ErrorPage::CertificateRevoked:
        case ErrorPage::CertificateKnownInterceptionBlocked:
            return QStringLiteral("This certificate is known to be invalid or used for "
                                 "interception, so the connection was refused.");
        case ErrorPage::SslPinnedKeyNotInCertificateChain:
            return QStringLiteral("The site's identity key is not the one it used before. "
                                 "Yozora stopped the connection instead of risking it.");
        case ErrorPage::TooManyRedirects:
            return QStringLiteral("The site keeps redirecting. This is usually a cookie problem "
                                 "on the site itself.");
        case ErrorPage::InvalidUrl:
            return QStringLiteral("The address is not a valid web address.");
        case ErrorPage::DisallowedUrlScheme:
        case ErrorPage::UnknownUrlScheme:
            return QStringLiteral("Yozora does not know how to open this kind of address.");
        case ErrorPage::UnsafePort:
            return QStringLiteral("Browsers refuse to open ports that are used by other "
                                 "kinds of software. Use the standard web port instead.");
        case ErrorPage::BlockedByClient:
        case ErrorPage::BlockedByOrb:
            return QStringLiteral("This request was blocked before it reached the site.");
        case ErrorPage::CacheMiss:
            return QStringLiteral("There is no cached copy of this page.");
        default:
            return QStringLiteral("The page did not load. Try again, or check your connection.");
    }
}

QString titleForDomain(int domain)
{
    switch (domain) {
        case ErrorPage::DomainCertificate:
            return QStringLiteral("Secure connection failed");
        case ErrorPage::DomainHttp:
            return QStringLiteral("The site returned an error");
        case ErrorPage::DomainDns:
            return QStringLiteral("Site not found");
        case ErrorPage::DomainConnection:
            return QStringLiteral("Cannot reach the site");
        case ErrorPage::DomainInternal:
            return QStringLiteral("Internal error");
        default:
            return QStringLiteral("Cannot open the page");
    }
}

QString hintForDomain(int domain)
{
    switch (domain) {
        case ErrorPage::DomainCertificate:
            return QStringLiteral("Yozora could not confirm that this site's certificate is "
                                 "valid, so the page was not loaded. Certificate warnings are "
                                 "never ignored in Yozora.");
        case ErrorPage::DomainHttp:
            return QStringLiteral("The site answered with an error instead of a page. Reload, or "
                                 "try again later.");
        case ErrorPage::DomainDns:
            return QStringLiteral("Yozora could not look up this site's address. Check the "
                                 "address for a typo.");
        case ErrorPage::DomainConnection:
            return QStringLiteral("The connection to this site could not be completed. Check "
                                 "your connection and try again.");
        case ErrorPage::DomainInternal:
            return QStringLiteral("Something went wrong inside the page. Reload to try again.");
        default:
            return QStringLiteral("The page did not load.");
    }
}

QString moonGlyph()
{
    // U+263E LAST QUARTER MOON - the one nod to the name, kept subtle.
    return QStringLiteral("\xE2\x98\xBE");
}

QString document(const QString& host, const QString& title, const QString& hint,
                 const QString& detail, bool dark)
{
    const QString detailHtml = detail.isEmpty()
        ? QString()
        : QStringLiteral("<p class=\"detail\">%1</p>").arg(escaped(detail));

    return QStringLiteral(R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>%TITLE%</title>
<style>
%STYLE%
body { display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }
.card { max-width: 540px; padding: 48px; text-align: center; }
.glyph { font-size: 40px; line-height: 1; color: var(--muted); opacity: 0.7; margin-bottom: 20px; }
h1 { font-size: 22px; font-weight: 600; margin: 0 0 16px; letter-spacing: -0.01em; }
.target {
  display: inline-block; max-width: 100%; margin-bottom: 20px; padding: 7px 14px;
  background: var(--surface); border: 1px solid var(--border); border-radius: 8px;
  color: var(--muted); font-size: 13px;
  overflow: hidden; text-overflow: ellipsis; white-space: nowrap;
}
.hint { font-size: 14px; line-height: 1.65; color: var(--muted); margin: 0 0 12px; }
.detail { font-size: 12px; color: var(--muted); opacity: 0.75; margin: 0 0 24px; word-break: break-word; }
.actions { display: flex; gap: 10px; justify-content: center; }
button {
  font: inherit; font-size: 13px; padding: 8px 18px; border-radius: 8px; cursor: pointer;
  border: 1px solid var(--border); background: var(--surface); color: var(--text);
}
button.primary { background: var(--accent); border-color: var(--accent); color: var(--accent-text); }
button:hover { filter: brightness(1.1); }
.footer { margin-top: 30px; font-size: 11px; letter-spacing: 0.08em; text-transform: uppercase; color: var(--muted); opacity: 0.55; }
</style>
</head>
<body>
  <div class="card">
    <div class="glyph">%GLYPH%</div>
    <h1>%TITLE%</h1>
    <div class="target" title="%HOST%">%HOST%</div>
    <p class="hint">%HINT%</p>
    %DETAIL%
    <div class="actions">
      <button class="primary" onclick="history.length &gt; 1 ? history.back() : location.reload()">Go back</button>
      <button onclick="location.reload()">Try again</button>
    </div>
    <div class="footer">Yozora</div>
  </div>
</body>
</html>)HTML")
        .replace(QStringLiteral("%STYLE%"), Theme::htmlStyle(dark))
        .replace(QStringLiteral("%TITLE%"), escaped(title))
        .replace(QStringLiteral("%HOST%"), escaped(host))
        .replace(QStringLiteral("%HINT%"), escaped(hint))
        .replace(QStringLiteral("%DETAIL%"), detailHtml)
        .replace(QStringLiteral("%GLYPH%"), moonGlyph());
}

}  // namespace

QString ErrorPage::titleFor(int domain, int errorCode)
{
    if (domain == DomainNone) {
        return QStringLiteral("Cannot open the page");
    }
    const QString net = titleForNet(errorCode);
    return net == QStringLiteral("Cannot open the page") ? titleForDomain(domain) : net;
}

QString ErrorPage::hintFor(int domain, int errorCode)
{
    if (domain == DomainNone) {
        return QStringLiteral("The page did not load.");
    }
    const QString net = hintForNet(errorCode);
    if (net.startsWith(QLatin1String("The page did not load"))) {
        return hintForDomain(domain);
    }
    return net;
}

QString ErrorPage::html(const QUrl& url, int domain, int errorCode, const QString& errorString,
                        bool dark)
{
    const QString host =
        url.host().isEmpty() ? (url.isValid() ? url.toString() : QStringLiteral("about:blank"))
                            : url.host();
    return document(host, titleFor(domain, errorCode), hintFor(domain, errorCode), errorString,
                    dark);
}

QString ErrorPage::htmlForHost(const QString& host, const QString& reason, bool dark)
{
    return document(host, QStringLiteral("Site not found"),
                    QStringLiteral("Yozora could not find this site on the network. Check the "
                                   "address for a typo, or check your connection."),
                    reason, dark);
}

}  // namespace yozora
