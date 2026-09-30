// SPDX-License-Identifier: MIT
#pragma once

#include "home/HomeWidget.h"

#include <QJsonArray>
#include <QList>
#include <QString>
#include <QUrl>

class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QNetworkReply;
class QTextEdit;
class QTimer;
class QGridLayout;

namespace yozora {

// The hero widget: wordmark, tagline and a large search field that also accepts
// URLs.
class SearchWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit SearchWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("search"); }
    [[nodiscard]] QString displayName() const override { return tr("Search"); }
    [[nodiscard]] QSize sizeHint() const override;
    void focusField();
signals:
    void searchRequested(const QString& text);
private:
    QLineEdit* m_field = nullptr;
};

// Pinned sites as a grid of rounded tiles.
class SitesWidget : public HomeWidget {
    Q_OBJECT
public:
    struct Site {
        QString url;
        QString title;
    };
    explicit SitesWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("sites"); }
    [[nodiscard]] QString displayName() const override { return tr("Sites"); }
    [[nodiscard]] QSize sizeHint() const override;
signals:
    void openUrl(const QUrl& url);
protected:
    [[nodiscard]] QJsonObject saveSettings() const override;
    void loadSettings(const QJsonObject& object) override;
private:
    void rebuild();
    void addSiteDialog();
    void removeSite(const QString& url);
    QGridLayout* m_grid = nullptr;
    QList<Site> m_sites;
};

// Bookmarks, read from the shared store.
class BookmarksWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit BookmarksWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("bookmarks"); }
    [[nodiscard]] QString displayName() const override { return tr("Bookmarks"); }
    [[nodiscard]] QSize sizeHint() const override;
signals:
    void openUrl(const QUrl& url);
private:
    void refresh();
    QWidget* m_list = nullptr;
};

// Browsing history, read from the shared store.
class HistoryWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit HistoryWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("history"); }
    [[nodiscard]] QString displayName() const override { return tr("Recently visited"); }
    [[nodiscard]] QSize sizeHint() const override;
signals:
    void openUrl(const QUrl& url);
private:
    void refresh();
    QWidget* m_list = nullptr;
};

// A local scratchpad. Content is saved with the layout.
class NotesWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit NotesWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("notes"); }
    [[nodiscard]] QString displayName() const override { return tr("Notes"); }
    [[nodiscard]] QSize sizeHint() const override;
protected:
    [[nodiscard]] QJsonObject saveSettings() const override;
    void loadSettings(const QJsonObject& object) override;
private:
    QTextEdit* m_edit = nullptr;
};

// Current weather. Opt-in: it only makes a request once the user sets a city.
class WeatherWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit WeatherWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("weather"); }
    [[nodiscard]] QString displayName() const override { return tr("Weather"); }
    [[nodiscard]] QSize sizeHint() const override;
protected:
    [[nodiscard]] QJsonObject saveSettings() const override;
    void loadSettings(const QJsonObject& object) override;
private:
    void beginFetch();
    void fetchForecast(double latitude, double longitude);
    void showError(const QString& message);
    QLabel* m_temp = nullptr;
    QLabel* m_detail = nullptr;
    QLineEdit* m_city = nullptr;
    QNetworkAccessManager* m_network = nullptr;
    QString m_cityName;
};

// Time and date.
class ClockWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit ClockWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("clock"); }
    [[nodiscard]] QString displayName() const override { return tr("Clock"); }
    [[nodiscard]] QSize sizeHint() const override;
private:
    void tick();
    QLabel* m_time = nullptr;
    QLabel* m_date = nullptr;
};

// Breaks the widget set into groups of shortcuts, shown as chips.
class QuickAccessWidget : public HomeWidget {
    Q_OBJECT
public:
    struct Shortcut {
        QString label;
        QString url;
    };
    explicit QuickAccessWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("quickaccess"); }
    [[nodiscard]] QString displayName() const override { return tr("Quick access"); }
    [[nodiscard]] QSize sizeHint() const override;
signals:
    void openUrl(const QUrl& url);
protected:
    [[nodiscard]] QJsonObject saveSettings() const override;
    void loadSettings(const QJsonObject& object) override;
private:
    void rebuild();
    void addShortcutDialog();
    QWidget* m_row = nullptr;
    QList<Shortcut> m_items;
};

// A compact overview: tabs, bookmarks, history and blocked trackers.
class StatsWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit StatsWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("stats"); }
    [[nodiscard]] QString displayName() const override { return tr("Quick stats"); }
    [[nodiscard]] QSize sizeHint() const override;
private:
    void refresh();
    QWidget* makeRow(const QString& label, QLabel** valueOut, const QString& glyph);
    QLabel* m_tabs = nullptr;
    QLabel* m_bookmarks = nullptr;
    QLabel* m_history = nullptr;
    QLabel* m_blocked = nullptr;
};

// A short statement of Yozora's privacy stance, with a link to the settings.
class PrivacyWidget : public HomeWidget {
    Q_OBJECT
public:
    explicit PrivacyWidget(const HomeContext& context, QWidget* parent = nullptr);
    [[nodiscard]] QString typeId() const override { return QStringLiteral("privacy"); }
    [[nodiscard]] QString displayName() const override { return tr("Privacy"); }
    [[nodiscard]] QSize sizeHint() const override;
};

}  // namespace yozora
