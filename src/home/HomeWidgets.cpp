// SPDX-License-Identifier: MIT
#include "home/HomeWidgets.h"

#include "core/BookmarkStore.h"
#include "core/HistoryStore.h"

#include <QDateTime>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QUrlQuery>
#include <QVBoxLayout>

namespace yozora {

namespace {

QString hostOf(const QString& url)
{
    QString host = QUrl(url).host();
    if (host.startsWith(QLatin1String("www."))) {
        host.remove(0, 4);
    }
    return host.isEmpty() ? url : host;
}

// A round, coloured tile icon with the first letter of the site.
QIcon letterAvatar(const QString& text, const QString& seed)
{
    const int hue = static_cast<int>(qHash(seed) % 360);
    const QColor color = QColor::fromHsl(hue, 150, 132);
    QPixmap pixmap(96, 96);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawRoundedRect(QRectF(6, 6, 84, 84), 24, 24);
    QFont font;
    font.setPointSizeF(36);
    font.setWeight(QFont::DemiBold);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter,
                     QString(text.isEmpty() ? QStringLiteral("?") : text.left(1).toUpper()));
    return QIcon(pixmap);
}

QPushButton* makeListRow(const QString& title, const QString& host)
{
    auto* row = new QPushButton;
    row->setObjectName(QStringLiteral("listRow"));
    row->setCursor(Qt::PointingHandCursor);
    row->setText(host.isEmpty() ? title : QStringLiteral("%1\n%2").arg(title, host));
    row->setFlat(true);
    return row;
}

QString wmoText(int code)
{
    if (code == 0) return QObject::tr("Clear");
    if (code <= 3) return QObject::tr("Partly cloudy");
    if (code <= 48) return QObject::tr("Fog");
    if (code <= 57) return QObject::tr("Drizzle");
    if (code <= 67) return QObject::tr("Rain");
    if (code <= 77) return QObject::tr("Snow");
    if (code <= 82) return QObject::tr("Showers");
    if (code <= 86) return QObject::tr("Snow showers");
    return QObject::tr("Thunderstorm");
}

}  // namespace

// ---------------------------------------------------------------------------
// Search
// ---------------------------------------------------------------------------

SearchWidget::SearchWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setProperty("hero", true);
    auto* layout = bodyLayout();
    layout->setAlignment(Qt::AlignCenter);

    auto* wordmark = new QLabel(QStringLiteral("\u2726  YOZORA"), body());
    wordmark->setObjectName(QStringLiteral("heroWordmark"));
    wordmark->setAlignment(Qt::AlignCenter);
    QFont wordFont = wordmark->font();
    wordFont.setPointSizeF(27);
    wordFont.setWeight(QFont::DemiBold);
    wordFont.setLetterSpacing(QFont::AbsoluteSpacing, 7);
    wordmark->setFont(wordFont);

    auto* tagline = new QLabel(tr("Your world. Your browser."), body());
    tagline->setObjectName(QStringLiteral("cardSubtle"));
    tagline->setAlignment(Qt::AlignCenter);

    m_field = new QLineEdit(body());
    m_field->setObjectName(QStringLiteral("heroSearch"));
    m_field->setPlaceholderText(tr("Search the web or enter a URL..."));
    m_field->setMinimumWidth(560);
    m_field->setMaximumWidth(760);
    m_field->setFixedHeight(50);
    connect(m_field, &QLineEdit::returnPressed, this, [this] {
        const QString text = m_field->text().trimmed();
        if (!text.isEmpty()) {
            emit searchRequested(text);
        }
    });

    layout->addStretch(1);
    layout->addWidget(wordmark, 0, Qt::AlignHCenter);
    layout->addWidget(tagline, 0, Qt::AlignHCenter);
    layout->addSpacing(20);
    layout->addWidget(m_field, 0, Qt::AlignHCenter);
    layout->addStretch(1);
}

QSize SearchWidget::sizeHint() const
{
    return {640, 226};
}

void SearchWidget::focusField()
{
    m_field->setFocus(Qt::OtherFocusReason);
    m_field->selectAll();
}

// ---------------------------------------------------------------------------
// Pinned sites
// ---------------------------------------------------------------------------

SitesWidget::SitesWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setTitle(tr("Pinned sites"));
    m_grid = new QGridLayout;
    m_grid->setSpacing(12);
    m_grid->setContentsMargins(0, 0, 0, 0);
    bodyLayout()->addLayout(m_grid);

    m_sites = {
        {QStringLiteral("https://duckduckgo.com"), QStringLiteral("DuckDuckGo")},
        {QStringLiteral("https://github.com"), QStringLiteral("GitHub")},
        {QStringLiteral("https://www.youtube.com"), QStringLiteral("YouTube")},
        {QStringLiteral("https://en.wikipedia.org"), QStringLiteral("Wikipedia")},
        {QStringLiteral("https://www.reddit.com"), QStringLiteral("Reddit")},
        {QStringLiteral("https://news.ycombinator.com"), QStringLiteral("Hacker News")},
    };
    rebuild();
}

void SitesWidget::rebuild()
{
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    constexpr int kColumns = 6;
    int index = 0;
    for (const Site& site : m_sites) {
        auto* tile = new QToolButton(body());
        tile->setObjectName(QStringLiteral("siteTile"));
        tile->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tile->setIcon(letterAvatar(site.title, site.url));
        tile->setIconSize(QSize(52, 52));
        tile->setText(site.title);
        tile->setToolTip(site.url);
        tile->setCursor(Qt::PointingHandCursor);
        tile->setFixedSize(140, 104);
        tile->setContextMenuPolicy(Qt::CustomContextMenu);
        const QString url = site.url;
        connect(tile, &QToolButton::clicked, this,
                [this, url] { emit openUrl(QUrl(url)); });
        connect(tile, &QToolButton::customContextMenuRequested, this, [this, url](const QPoint& p) {
            QMenu menu;
            QAction* remove = menu.addAction(tr("Remove"));
            if (menu.exec(mapToGlobal(p)) == remove) {
                removeSite(url);
            }
        });
        m_grid->addWidget(tile, index / kColumns, index % kColumns);
        ++index;
    }

    auto* add = new QToolButton(body());
    add->setObjectName(QStringLiteral("siteTile"));
    add->setToolButtonStyle(Qt::ToolButtonTextOnly);
    add->setText(QStringLiteral("+"));
    add->setToolTip(tr("Add a site"));
    add->setCursor(Qt::PointingHandCursor);
    add->setFixedSize(140, 104);
    QFont addFont = add->font();
    addFont.setPointSizeF(22);
    add->setFont(addFont);
    connect(add, &QToolButton::clicked, this, &SitesWidget::addSiteDialog);
    m_grid->addWidget(add, index / kColumns, index % kColumns);

    emit changed();
}

void SitesWidget::addSiteDialog()
{
    bool ok = false;
    const QString input = QInputDialog::getText(this, tr("Add site"), tr("Address:"),
                                                QLineEdit::Normal, QString(), &ok);
    if (!ok || input.trimmed().isEmpty()) {
        return;
    }
    QString url = input.trimmed();
    if (!url.contains(QLatin1String("://"))) {
        url.prepend(QLatin1String("https://"));
    }
    m_sites.append({url, hostOf(url)});
    rebuild();
}

void SitesWidget::removeSite(const QString& url)
{
    for (int i = 0; i < m_sites.size(); ++i) {
        if (m_sites.at(i).url == url) {
            m_sites.removeAt(i);
            rebuild();
            return;
        }
    }
}

QJsonObject SitesWidget::saveSettings() const
{
    QJsonArray array;
    for (const Site& site : m_sites) {
        QJsonObject object;
        object.insert(QStringLiteral("url"), site.url);
        object.insert(QStringLiteral("title"), site.title);
        array.append(object);
    }
    QJsonObject settings;
    settings.insert(QStringLiteral("sites"), array);
    return settings;
}

void SitesWidget::loadSettings(const QJsonObject& object)
{
    const QJsonArray array = object.value(QStringLiteral("sites")).toArray();
    m_sites.clear();
    for (const QJsonValue& value : array) {
        const QJsonObject entry = value.toObject();
        const QString url = entry.value(QStringLiteral("url")).toString();
        if (!url.isEmpty()) {
            m_sites.append({url, entry.value(QStringLiteral("title")).toString()});
        }
    }
    rebuild();
}

QSize SitesWidget::sizeHint() const
{
    const int rows = (static_cast<int>(m_sites.size()) + 1 + 5) / 6;
    return {640, 40 + rows * 98};
}

// ---------------------------------------------------------------------------
// Bookmarks
// ---------------------------------------------------------------------------

BookmarksWidget::BookmarksWidget(const HomeContext& homeContext, QWidget* parent)
    : HomeWidget(homeContext, parent)
{
    setTitle(tr("Bookmarks"));

    auto* viewAll = new QToolButton(this);
    viewAll->setObjectName(QStringLiteral("cardAction"));
    viewAll->setText(tr("View all"));
    viewAll->setCursor(Qt::PointingHandCursor);
    viewAll->setFocusPolicy(Qt::NoFocus);
    connect(viewAll, &QToolButton::clicked, this, [this] {
        if (context().openBookmarks) {
            context().openBookmarks();
        }
    });
    addHeaderAction(viewAll);

    m_list = new QWidget(body());
    auto* layout = new QVBoxLayout(m_list);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    bodyLayout()->addWidget(m_list);

    if (homeContext.bookmarks) {
        connect(homeContext.bookmarks, &BookmarkStore::changed, this, &BookmarksWidget::refresh);
    }
    refresh();
}

void BookmarksWidget::refresh()
{
    auto* layout = qobject_cast<QVBoxLayout*>(m_list->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const QList<Bookmark> items = context().bookmarks ? context().bookmarks->all() : QList<Bookmark>{};
    if (items.isEmpty()) {
        auto* empty = new QLabel(tr("No bookmarks yet. Press the star in the toolbar."), m_list);
        empty->setObjectName(QStringLiteral("cardSubtle"));
        empty->setWordWrap(true);
        layout->addWidget(empty);
        return;
    }

    int shown = 0;
    for (const Bookmark& bookmark : items) {
        if (shown++ >= 6) {
            break;
        }
        auto* row = makeListRow(bookmark.title, hostOf(bookmark.url));
        const QString url = bookmark.url;
        connect(row, &QPushButton::clicked, this, [this, url] { emit openUrl(QUrl(url)); });
        row->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(row, &QPushButton::customContextMenuRequested, this, [this, url, row](const QPoint& p) {
            QMenu menu;
            QAction* remove = menu.addAction(tr("Remove bookmark"));
            if (menu.exec(row->mapToGlobal(p)) == remove && context().bookmarks) {
                context().bookmarks->remove(url);
            }
        });
        layout->addWidget(row);
    }
}

QSize BookmarksWidget::sizeHint() const
{
    return {320, 260};
}

// ---------------------------------------------------------------------------
// History
// ---------------------------------------------------------------------------

HistoryWidget::HistoryWidget(const HomeContext& homeContext, QWidget* parent)
    : HomeWidget(homeContext, parent)
{
    setTitle(tr("Recently visited"));

    auto* viewAll = new QToolButton(this);
    viewAll->setObjectName(QStringLiteral("cardAction"));
    viewAll->setText(tr("View all"));
    viewAll->setCursor(Qt::PointingHandCursor);
    viewAll->setFocusPolicy(Qt::NoFocus);
    connect(viewAll, &QToolButton::clicked, this, [this] {
        if (context().openHistory) {
            context().openHistory();
        }
    });
    addHeaderAction(viewAll);

    m_list = new QWidget(body());
    auto* layout = new QVBoxLayout(m_list);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(2);
    bodyLayout()->addWidget(m_list);

    if (homeContext.history) {
        connect(homeContext.history, &HistoryStore::changed, this, &HistoryWidget::refresh);
    }
    refresh();
}

void HistoryWidget::refresh()
{
    auto* layout = qobject_cast<QVBoxLayout*>(m_list->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    const QList<HistoryEntry> items =
        context().history ? context().history->recent(6) : QList<HistoryEntry>{};
    if (items.isEmpty()) {
        auto* empty = new QLabel(tr("Pages you visit will show up here."), m_list);
        empty->setObjectName(QStringLiteral("cardSubtle"));
        empty->setWordWrap(true);
        layout->addWidget(empty);
        return;
    }

    for (const HistoryEntry& entry : items) {
        auto* row = makeListRow(entry.title, hostOf(entry.url));
        const QString url = entry.url;
        connect(row, &QPushButton::clicked, this, [this, url] { emit openUrl(QUrl(url)); });
        layout->addWidget(row);
    }
}

QSize HistoryWidget::sizeHint() const
{
    return {320, 260};
}

// ---------------------------------------------------------------------------
// Notes
// ---------------------------------------------------------------------------

NotesWidget::NotesWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setTitle(tr("Notes"));
    m_edit = new QTextEdit(body());
    m_edit->setPlaceholderText(tr("Jot something down..."));
    m_edit->setFrameShape(QFrame::NoFrame);
    m_edit->setStyleSheet(QStringLiteral("background: transparent;"));
    connect(m_edit, &QTextEdit::textChanged, this, &NotesWidget::changed);
    bodyLayout()->addWidget(m_edit);
}

QJsonObject NotesWidget::saveSettings() const
{
    QJsonObject settings;
    settings.insert(QStringLiteral("text"), m_edit->toPlainText());
    return settings;
}

void NotesWidget::loadSettings(const QJsonObject& object)
{
    m_edit->setPlainText(object.value(QStringLiteral("text")).toString());
}

QSize NotesWidget::sizeHint() const
{
    return {300, 240};
}

// ---------------------------------------------------------------------------
// Weather
// ---------------------------------------------------------------------------

WeatherWidget::WeatherWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
    , m_network(new QNetworkAccessManager(this))
{
    setTitle(tr("Weather"));

    m_temp = new QLabel(QStringLiteral("--"), body());
    QFont tempFont = m_temp->font();
    tempFont.setPointSizeF(30);
    tempFont.setWeight(QFont::Light);
    m_temp->setFont(tempFont);
    m_detail = new QLabel(tr("Set a city to see the weather."), body());
    m_detail->setObjectName(QStringLiteral("cardSubtle"));
    m_detail->setWordWrap(true);

    m_city = new QLineEdit(body());
    m_city->setPlaceholderText(tr("City (e.g. Berlin)"));
    connect(m_city, &QLineEdit::returnPressed, this, [this] {
        m_cityName = m_city->text().trimmed();
        if (!m_cityName.isEmpty()) {
            emit changed();
            beginFetch();
        }
    });

    bodyLayout()->addWidget(m_temp);
    bodyLayout()->addWidget(m_detail);
    bodyLayout()->addStretch(1);
    bodyLayout()->addWidget(m_city);
}

QJsonObject WeatherWidget::saveSettings() const
{
    QJsonObject settings;
    settings.insert(QStringLiteral("city"), m_cityName);
    return settings;
}

void WeatherWidget::loadSettings(const QJsonObject& object)
{
    m_cityName = object.value(QStringLiteral("city")).toString();
    if (!m_cityName.isEmpty()) {
        m_city->setText(m_cityName);
        beginFetch();
    }
}

void WeatherWidget::showError(const QString& message)
{
    m_temp->setText(QStringLiteral("--"));
    m_detail->setText(message);
}

void WeatherWidget::beginFetch()
{
    // Opt-in and documented: only runs once the user has named a city.
    m_detail->setText(tr("Loading..."));
    QUrl url(QStringLiteral("https://geocoding-api.open-meteo.com/v1/search"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("name"), m_cityName);
    query.addQueryItem(QStringLiteral("count"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("language"), QStringLiteral("en"));
    query.addQueryItem(QStringLiteral("format"), QStringLiteral("json"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            showError(tr("Could not reach the weather service."));
            return;
        }
        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray results =
            document.object().value(QStringLiteral("results")).toArray();
        if (results.isEmpty()) {
            showError(tr("City not found."));
            return;
        }
        const QJsonObject place = results.first().toObject();
        m_cityName = place.value(QStringLiteral("name")).toString(m_cityName);
        m_city->setText(m_cityName);
        fetchForecast(place.value(QStringLiteral("latitude")).toDouble(),
                      place.value(QStringLiteral("longitude")).toDouble());
    });
}

void WeatherWidget::fetchForecast(double latitude, double longitude)
{
    QUrl url(QStringLiteral("https://api.open-meteo.com/v1/forecast"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("latitude"), QString::number(latitude, 'f', 4));
    query.addQueryItem(QStringLiteral("longitude"), QString::number(longitude, 'f', 4));
    query.addQueryItem(QStringLiteral("current"), QStringLiteral("temperature_2m,weather_code"));
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    QNetworkReply* reply = m_network->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply] {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            showError(tr("Could not load the weather."));
            return;
        }
        const QJsonObject current = QJsonDocument::fromJson(reply->readAll())
                                        .object()
                                        .value(QStringLiteral("current"))
                                        .toObject();
        const double temperature = current.value(QStringLiteral("temperature_2m")).toDouble();
        const int code = current.value(QStringLiteral("weather_code")).toInt();
        m_temp->setText(QStringLiteral("%1\u00B0").arg(qRound(temperature)));
        m_detail->setText(QStringLiteral("%1 in %2").arg(wmoText(code), m_cityName));
    });
}

QSize WeatherWidget::sizeHint() const
{
    return {300, 240};
}

// ---------------------------------------------------------------------------
// Clock
// ---------------------------------------------------------------------------

ClockWidget::ClockWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setTitle(tr("Time"));

    m_time = new QLabel(body());
    QFont timeFont = m_time->font();
    timeFont.setPointSizeF(34);
    timeFont.setWeight(QFont::Light);
    m_time->setFont(timeFont);

    m_date = new QLabel(body());
    m_date->setObjectName(QStringLiteral("cardSubtle"));

    bodyLayout()->addStretch(1);
    bodyLayout()->addWidget(m_time);
    bodyLayout()->addWidget(m_date);
    bodyLayout()->addStretch(1);

    auto* timer = new QTimer(this);
    timer->setInterval(10000);
    connect(timer, &QTimer::timeout, this, &ClockWidget::tick);
    timer->start();
    tick();
}

void ClockWidget::tick()
{
    const QDateTime now = QDateTime::currentDateTime();
    m_time->setText(now.toString(QStringLiteral("HH:mm")));
    m_date->setText(now.toString(QStringLiteral("dddd, d MMM")));
}

QSize ClockWidget::sizeHint() const
{
    return {300, 200};
}

// ---------------------------------------------------------------------------
// Quick access
// ---------------------------------------------------------------------------

QuickAccessWidget::QuickAccessWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setTitle(tr("Quick access"));

    m_row = new QWidget(body());
    auto* rowLayout = new QHBoxLayout(m_row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(10);
    bodyLayout()->addWidget(m_row);

    m_items = {
        {tr("Development"), QStringLiteral("https://github.com")},
        {tr("Gaming"), QStringLiteral("https://store.steampowered.com")},
        {tr("Media"), QStringLiteral("https://www.youtube.com")},
        {tr("Tools"), QStringLiteral("https://news.ycombinator.com")},
        {tr("Education"), QStringLiteral("https://en.wikipedia.org")},
    };
    rebuild();
}

void QuickAccessWidget::rebuild()
{
    auto* layout = qobject_cast<QHBoxLayout*>(m_row->layout());
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    for (const Shortcut& shortcut : m_items) {
        auto* chip = new QPushButton(shortcut.label, m_row);
        chip->setObjectName(QStringLiteral("chip"));
        chip->setCursor(Qt::PointingHandCursor);
        const QString url = shortcut.url;
        connect(chip, &QPushButton::clicked, this, [this, url] { emit openUrl(QUrl(url)); });
        chip->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(chip, &QPushButton::customContextMenuRequested, this, [this, url, chip](const QPoint& p) {
            QMenu menu;
            QAction* remove = menu.addAction(tr("Remove"));
            if (menu.exec(chip->mapToGlobal(p)) == remove) {
                for (int i = 0; i < m_items.size(); ++i) {
                    if (m_items.at(i).url == url) {
                        m_items.removeAt(i);
                        rebuild();
                        break;
                    }
                }
            }
        });
        layout->addWidget(chip);
    }

    auto* add = new QPushButton(QStringLiteral("+"), m_row);
    add->setObjectName(QStringLiteral("chip"));
    add->setToolTip(tr("Add a shortcut"));
    add->setCursor(Qt::PointingHandCursor);
    connect(add, &QPushButton::clicked, this, &QuickAccessWidget::addShortcutDialog);
    layout->addWidget(add);
    layout->addStretch(1);

    emit changed();
}

void QuickAccessWidget::addShortcutDialog()
{
    bool ok = false;
    const QString input = QInputDialog::getText(this, tr("Add shortcut"), tr("Address:"),
                                                QLineEdit::Normal, QString(), &ok);
    if (!ok || input.trimmed().isEmpty()) {
        return;
    }
    QString url = input.trimmed();
    if (!url.contains(QLatin1String("://"))) {
        url.prepend(QLatin1String("https://"));
    }
    m_items.append({hostOf(url), url});
    rebuild();
}

QJsonObject QuickAccessWidget::saveSettings() const
{
    QJsonArray array;
    for (const Shortcut& shortcut : m_items) {
        QJsonObject object;
        object.insert(QStringLiteral("label"), shortcut.label);
        object.insert(QStringLiteral("url"), shortcut.url);
        array.append(object);
    }
    QJsonObject settings;
    settings.insert(QStringLiteral("items"), array);
    return settings;
}

void QuickAccessWidget::loadSettings(const QJsonObject& object)
{
    const QJsonArray array = object.value(QStringLiteral("items")).toArray();
    if (array.isEmpty()) {
        return;
    }
    m_items.clear();
    for (const QJsonValue& value : array) {
        const QJsonObject entry = value.toObject();
        const QString url = entry.value(QStringLiteral("url")).toString();
        if (!url.isEmpty()) {
            m_items.append({entry.value(QStringLiteral("label")).toString(), url});
        }
    }
    rebuild();
}

QSize QuickAccessWidget::sizeHint() const
{
    return {640, 116};
}

// ---------------------------------------------------------------------------
// Quick stats
// ---------------------------------------------------------------------------

StatsWidget::StatsWidget(const HomeContext& context, QWidget* parent)
    : HomeWidget(context, parent)
{
    setTitle(tr("Quick stats"));

    bodyLayout()->addWidget(makeRow(tr("Total tabs"), &m_tabs, QStringLiteral("\u25A2")));
    bodyLayout()->addWidget(makeRow(tr("Bookmarks"), &m_bookmarks, QStringLiteral("\u2691")));
    bodyLayout()->addWidget(makeRow(tr("History"), &m_history, QStringLiteral("\u25F7")));
    bodyLayout()->addWidget(makeRow(tr("Trackers blocked"), &m_blocked, QStringLiteral("\u25C8")));
    bodyLayout()->addStretch(1);

    if (context.bookmarks) {
        connect(context.bookmarks, &BookmarkStore::changed, this, &StatsWidget::refresh);
    }
    if (context.history) {
        connect(context.history, &HistoryStore::changed, this, &StatsWidget::refresh);
    }
    auto* timer = new QTimer(this);
    timer->setInterval(2000);
    connect(timer, &QTimer::timeout, this, &StatsWidget::refresh);
    timer->start();
    refresh();
}

QWidget* StatsWidget::makeRow(const QString& label, QLabel** valueOut, const QString& glyph)
{
    auto* row = new QWidget(body());
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    auto* glyphLabel = new QLabel(glyph, row);
    glyphLabel->setObjectName(QStringLiteral("cardSubtle"));
    layout->addWidget(glyphLabel);

    auto* text = new QLabel(label, row);
    layout->addWidget(text);
    layout->addStretch(1);

    auto* value = new QLabel(QStringLiteral("0"), row);
    value->setObjectName(QStringLiteral("statsValue"));
    layout->addWidget(value);
    *valueOut = value;
    return row;
}

void StatsWidget::refresh()
{
    const int tabs = context().openTabCount ? context().openTabCount() : 0;
    const int bookmarks = context().bookmarks ? context().bookmarks->count() : 0;
    const int history = context().history ? context().history->count() : 0;
    const quint64 blocked = context().blockedTrackerCount ? context().blockedTrackerCount() : 0;
    m_tabs->setText(QString::number(tabs));
    m_bookmarks->setText(QString::number(bookmarks));
    m_history->setText(QString::number(history));
    m_blocked->setText(QString::number(blocked));
}

QSize StatsWidget::sizeHint() const
{
    return {300, 240};
}

// ---------------------------------------------------------------------------
// Privacy
// ---------------------------------------------------------------------------

PrivacyWidget::PrivacyWidget(const HomeContext& homeContext, QWidget* parent)
    : HomeWidget(homeContext, parent)
{
    setTitle(tr("Privacy"));

    auto* glyph = new QLabel(QStringLiteral("\u2726"), body());
    QFont glyphFont = glyph->font();
    glyphFont.setPointSizeF(26);
    glyph->setFont(glyphFont);
    glyph->setObjectName(QStringLiteral("privacyGlyph"));

    auto* title = new QLabel(tr("Privacy first"), body());
    title->setObjectName(QStringLiteral("cardTitle"));
    auto* text = new QLabel(tr("Your data stays on your device. No telemetry, no account."), body());
    text->setObjectName(QStringLiteral("cardSubtle"));
    text->setWordWrap(true);

    auto* button = new QPushButton(tr("Review privacy"), body());
    connect(button, &QPushButton::clicked, this, [this] {
        if (context().openSettings) {
            context().openSettings();
        }
    });

    bodyLayout()->addWidget(glyph);
    bodyLayout()->addWidget(title);
    bodyLayout()->addWidget(text);
    bodyLayout()->addStretch(1);
    bodyLayout()->addWidget(button);
}

QSize PrivacyWidget::sizeHint() const
{
    return {300, 240};
}

}  // namespace yozora
