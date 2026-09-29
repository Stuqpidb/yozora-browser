// SPDX-License-Identifier: MIT
#include "core/Settings.h"

#include "core/SearchEngine.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>

namespace yozora {

class SettingsPrivate {
public:
    QSettings store;
    Settings::ThemeMode cachedTheme = Settings::ThemeMode::Dark;
    bool cacheValid = false;
};

namespace {
constexpr auto kKeySearchEngine = "browser/search_engine";
constexpr auto kKeyHomePage = "browser/home_page";
constexpr auto kKeyDownloadDir = "downloads/directory";
constexpr auto kKeyAskWhereToSave = "downloads/ask_where_to_save";
constexpr auto kKeyThemeMode = "appearance/theme_mode";
constexpr auto kKeyWindowGeometry = "window/geometry";
constexpr auto kKeyWindowState = "window/state";
constexpr auto kKeyRestoreSession = "session/restore_on_start";
}  // namespace

Settings::Settings(QObject* parent)
    : QObject(parent)
    , d(new SettingsPrivate)
{
}

Settings::~Settings() = default;

QString Settings::searchEngineId() const
{
    const QString fallback = SearchEngines::defaultId();
    const QString id = d->store.value(QLatin1String(kKeySearchEngine), fallback).toString();
    // Guard against a stale id after an update removed the engine.
    return SearchEngines::byId(id).id;
}

void Settings::setSearchEngineId(const QString& id)
{
    if (id.isEmpty() || searchEngineId() == id) {
        return;
    }
    d->store.setValue(QLatin1String(kKeySearchEngine), id);
    emit searchEngineChanged();
}

QString Settings::homePage() const
{
    return d->store.value(QLatin1String(kKeyHomePage), QStringLiteral("about:yozora")).toString();
}

void Settings::setHomePage(const QString& url)
{
    if (url.isEmpty() || homePage() == url) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyHomePage), url);
    emit homePageChanged();
}

QString Settings::downloadDirectory() const
{
    const QString stored = d->store.value(QLatin1String(kKeyDownloadDir)).toString();
    if (!stored.isEmpty() && QDir(stored).exists()) {
        return stored;
    }
    const QString fallback =
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (!fallback.isEmpty()) {
        return fallback;
    }
    return QDir::homePath();
}

void Settings::setDownloadDirectory(const QString& path)
{
    if (path.isEmpty() || downloadDirectory() == path) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyDownloadDir), path);
    emit downloadDirectoryChanged();
}

bool Settings::askWhereToSave() const
{
    return d->store.value(QLatin1String(kKeyAskWhereToSave), false).toBool();
}

void Settings::setAskWhereToSave(bool ask)
{
    d->store.setValue(QLatin1String(kKeyAskWhereToSave), ask);
}

Settings::ThemeMode Settings::themeMode() const
{
    if (!d->cacheValid) {
        const int raw = d->store.value(QLatin1String(kKeyThemeMode),
                                       static_cast<int>(ThemeMode::Dark))
                            .toInt();
        d->cachedTheme = (raw >= 0 && raw <= 2) ? static_cast<ThemeMode>(raw) : ThemeMode::Dark;
        d->cacheValid = true;
    }
    return d->cachedTheme;
}

void Settings::setThemeMode(ThemeMode mode)
{
    if (themeMode() == mode) {
        return;
    }
    d->store.setValue(QLatin1String(kKeyThemeMode), static_cast<int>(mode));
    d->cachedTheme = mode;
    d->cacheValid = true;
    emit themeModeChanged();
}

QByteArray Settings::windowGeometry() const
{
    return d->store.value(QLatin1String(kKeyWindowGeometry)).toByteArray();
}

void Settings::setWindowGeometry(const QByteArray& geometry)
{
    d->store.setValue(QLatin1String(kKeyWindowGeometry), geometry);
}

QByteArray Settings::windowState() const
{
    return d->store.value(QLatin1String(kKeyWindowState)).toByteArray();
}

void Settings::setWindowState(const QByteArray& state)
{
    d->store.setValue(QLatin1String(kKeyWindowState), state);
}

bool Settings::restoreSessionOnStart() const
{
    return d->store.value(QLatin1String(kKeyRestoreSession), true).toBool();
}

void Settings::setRestoreSessionOnStart(bool restore)
{
    d->store.setValue(QLatin1String(kKeyRestoreSession), restore);
}

void Settings::resetToDefaults()
{
    d->store.clear();
    d->cacheValid = false;
    emit searchEngineChanged();
    emit homePageChanged();
    emit downloadDirectoryChanged();
    emit themeModeChanged();
}

}  // namespace yozora
