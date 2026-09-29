// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace yozora {

// Lightweight wrapper around QSettings. Every user visible preference goes
// through here so that adding a new setting later never requires touching the
// UI widgets.
class Settings : public QObject {
    Q_OBJECT

public:
    explicit Settings(QObject* parent = nullptr);
    ~Settings() override;

    [[nodiscard]] QString searchEngineId() const;
    void setSearchEngineId(const QString& id);

    // Page opened in a brand new tab.
    [[nodiscard]] QString homePage() const;
    void setHomePage(const QString& url);

    [[nodiscard]] QString downloadDirectory() const;
    void setDownloadDirectory(const QString& path);

    [[nodiscard]] bool askWhereToSave() const;
    void setAskWhereToSave(bool ask);

    enum class ThemeMode { Dark, Light, System };
    Q_ENUM(ThemeMode)

    [[nodiscard]] ThemeMode themeMode() const;
    void setThemeMode(ThemeMode mode);

    [[nodiscard]] QByteArray windowGeometry() const;
    void setWindowGeometry(const QByteArray& geometry);

    [[nodiscard]] QByteArray windowState() const;
    void setWindowState(const QByteArray& state);

    [[nodiscard]] bool restoreSessionOnStart() const;
    void setRestoreSessionOnStart(bool restore);

    void resetToDefaults();

signals:
    void searchEngineChanged();
    void homePageChanged();
    void downloadDirectoryChanged();
    void themeModeChanged();

private:
    class QScopedPointer<class SettingsPrivate> d;
};

}  // namespace yozora
