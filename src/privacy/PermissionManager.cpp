// SPDX-License-Identifier: MIT
#include "privacy/PermissionManager.h"

#include "core/Settings.h"
#include "ui/PermissionPrompt.h"

#include <QDialog>
#include <QUrl>
#include <QWebEnginePage>
#include <QWebEnginePermission>
#include <QWebEngineProfile>
#include <QWidget>

namespace yozora {

namespace {

using PermissionType = QWebEnginePermission::PermissionType;

// Which permission types Chromium remembers between visits. These are exactly
// the types the engine documents as persistent; everything else has to be
// asked for again every time. Encoding the set here keeps the prompt wording
// honest without depending on an engine query.
bool isPersistentPermission(PermissionType type)
{
    switch (type) {
        case PermissionType::Notifications:
        case PermissionType::Geolocation:
        case PermissionType::ClipboardReadWrite:
        case PermissionType::LocalFontsAccess:
            return true;
        default:
            return false;
    }
}

QString featurePhrase(PermissionType type)
{
    switch (type) {
        case PermissionType::MediaAudioCapture:
            return QObject::tr("use your microphone");
        case PermissionType::MediaVideoCapture:
            return QObject::tr("use your camera");
        case PermissionType::MediaAudioVideoCapture:
            return QObject::tr("use your camera and microphone");
        case PermissionType::MouseLock:
            return QObject::tr("lock your mouse pointer");
        case PermissionType::Notifications:
            return QObject::tr("show notifications");
        case PermissionType::Geolocation:
            return QObject::tr("know your location");
        case PermissionType::ClipboardReadWrite:
            return QObject::tr("read and write your clipboard");
        case PermissionType::LocalFontsAccess:
            return QObject::tr("use the fonts installed on your computer");
        case PermissionType::DesktopVideoCapture:
            return QObject::tr("share your screen");
        case PermissionType::DesktopAudioVideoCapture:
            return QObject::tr("share your screen and system audio");
        default:
            return QObject::tr("use a protected feature");
    }
}

}  // namespace

PermissionManager::PermissionManager(QWebEngineProfile* profile, Settings* settings, QObject* parent)
    : QObject(parent)
    , m_profile(profile)
    , m_settings(settings)
{
    // Turning notifications off must also revoke anything already granted, so a
    // site cannot keep notifying after the switch was flipped.
    if (m_settings && m_profile) {
        connect(m_settings, &Settings::notificationsEnabledChanged, this, [this] {
            if (m_settings && !m_settings->notificationsEnabled()) {
                const QList<QWebEnginePermission> stored = m_profile->listAllPermissions();
                for (const QWebEnginePermission& permission : stored) {
                    if (permission.permissionType() == PermissionType::Notifications
                        && permission.isValid()) {
                        permission.reset();
                    }
                }
            }
        });
    }
}

void PermissionManager::attachPage(QWebEnginePage* page)
{
    if (!page) {
        return;
    }
    connect(page, &QWebEnginePage::permissionRequested, this,
            [this, page](QWebEnginePermission permission) {
                // Parent the prompt to the window that owns the requesting
                // page, so it is modal to the right window.
                QWidget* window = nullptr;
                if (auto* widget = qobject_cast<QWidget*>(page->parent())) {
                    window = widget->window();
                }
                handleRequest(permission, window);
            });
}

void PermissionManager::handleRequest(const QWebEnginePermission& permission, QWidget* parentWindow)
{
    if (!permission.isValid()) {
        return;
    }

    // Only real web origins may ask. Internal pages (about:, data:,
    // yozora-error:) never get a capability.
    const QUrl origin = permission.origin();
    const QString scheme = origin.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https")) {
        permission.deny();
        return;
    }

    const PermissionType type = permission.permissionType();
    if (type == PermissionType::Unsupported) {
        permission.deny();
        return;
    }

    // Screen sharing needs a picker that Yozora does not have yet. Refusing is
    // honest; faking a grant without a picker would hand over the whole screen.
    if (type == PermissionType::DesktopVideoCapture
        || type == PermissionType::DesktopAudioVideoCapture) {
        permission.deny();
        emit permissionDenied(origin.host(), QObject::tr("screen sharing"));
        return;
    }

    // The global notification switch overrides the per-site prompt.
    if (type == PermissionType::Notifications && m_settings
        && !m_settings->notificationsEnabled()) {
        permission.deny();
        emit permissionDenied(origin.host(), QObject::tr("notifications"));
        return;
    }

    PermissionPrompt prompt(parentWindow);
    prompt.setRequest(origin, featurePhrase(type), isPersistentPermission(type));

    const bool allow = (prompt.exec() == QDialog::Accepted);
    // The page may have navigated away while the prompt was open, in which case
    // the permission object is no longer valid and the decision is dropped.
    if (!permission.isValid()) {
        return;
    }
    if (allow) {
        permission.grant();
    } else {
        permission.deny();
    }
}

void PermissionManager::clearStoredPermissions()
{
    if (!m_profile) {
        return;
    }
    const QList<QWebEnginePermission> stored = m_profile->listAllPermissions();
    for (const QWebEnginePermission& permission : stored) {
        if (permission.isValid()) {
            permission.reset();
        }
    }
}

}  // namespace yozora
