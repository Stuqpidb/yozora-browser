// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>

class QWebEnginePage;
class QWebEnginePermission;
class QWebEngineProfile;
class QWidget;

namespace yozora {

class Settings;

// Turns Chromium's permission requests into a user decision.
//
// Qt 6.8 owns the permission store: a decision made here is remembered by the
// profile itself (per origin) for the permission types that are persistent, and
// forgotten for the ones that are not (camera, microphone, screen sharing,
// pointer lock). Yozora never pre-grants anything and never grants a capability
// to every site at once; each request is tied to one origin.
class PermissionManager : public QObject {
    Q_OBJECT

public:
    PermissionManager(QWebEngineProfile* profile, Settings* settings, QObject* parent = nullptr);

    // Routes a page's permission requests through the manager.
    void attachPage(QWebEnginePage* page);

    // Forgets every stored grant/deny for the profile, so the next request asks
    // again. Used by "Clear browsing data".
    void clearStoredPermissions();

signals:
    // A request was refused without showing a prompt (global switch or an
    // unsupported capability). The shell shows this in the status bar.
    void permissionDenied(const QString& origin, const QString& feature);

private:
    void handleRequest(const QWebEnginePermission& permission, QWidget* parentWindow);

    QWebEngineProfile* m_profile = nullptr;
    Settings* m_settings = nullptr;
};

}  // namespace yozora
