// SPDX-License-Identifier: MIT
#include "home/HomePage.h"

#include "app/AppPaths.h"
#include "core/Glass.h"
#include "core/NightSky.h"
#include "core/Theme.h"
#include "ui/GlassField.h"
#include "ui/Icons.h"

#include <QFile>
#include <QFont>
#include <QGridLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QSaveFile>
#include <QToolButton>
#include <QVBoxLayout>

#include <utility>

namespace yozora {

namespace {

constexpr int kTileSize = 88;
constexpr int kTileSpacing = 10;
constexpr int kFieldWidth = 604;
constexpr int kFieldHeight = 58;

// A round, coloured tile with the first letter of the site. The hue comes from
// the address, so the same site always looks the same.
QIcon letterAvatar(const QString& text, const QString& seed)
{
    const int hue = static_cast<int>(qHash(seed) % 360);
    const QColor color = QColor::fromHsl(hue, 148, 128);
    QPixmap pixmap(96, 96);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    // A superellipse, not a rounded square: this is the shape that makes a
    // grid of icons read as a home screen rather than as a list of buttons.
    painter.drawPath(Glass::squircle(QRectF(6, 6, 84, 84), 25));
    QFont font(Theme::displayFamily());
    font.setPointSizeF(36);
    font.setWeight(QFont::Bold);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(pixmap.rect(), Qt::AlignCenter,
                     QString(text.isEmpty() ? QStringLiteral("?") : text.left(1).toUpper()));
    return QIcon(pixmap);
}

QString hostOf(const QString& url)
{
    QString host = QUrl(url).host();
    if (host.startsWith(QLatin1String("www."))) {
        host.remove(0, 4);
    }
    return host.isEmpty() ? url : host;
}

QList<QPair<QString, QString>> defaultSites()
{
    return {
        {QStringLiteral("https://duckduckgo.com"), QStringLiteral("DuckDuckGo")},
        {QStringLiteral("https://github.com"), QStringLiteral("GitHub")},
        {QStringLiteral("https://www.youtube.com"), QStringLiteral("YouTube")},
        {QStringLiteral("https://en.wikipedia.org"), QStringLiteral("Wikipedia")},
        {QStringLiteral("https://www.reddit.com"), QStringLiteral("Reddit")},
        {QStringLiteral("https://news.ycombinator.com"), QStringLiteral("Hacker News")},
    };
}

}  // namespace

// ---------------------------------------------------------------------------
// Pinned sites
// ---------------------------------------------------------------------------

PinnedSites::PinnedSites(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("pinnedSites"));
    m_grid = new QGridLayout(this);
    m_grid->setSpacing(kTileSpacing);
    m_grid->setContentsMargins(0, 0, 0, 0);
    // Packed to the left and centred as a block: a row of icons, not a
    // justified table.
    m_grid->setAlignment(Qt::AlignCenter);
}

QSize PinnedSites::sizeHint() const
{
    const int count = m_sites.size() + 1;
    const int columns = qMax(1, qMin(count, 9));
    const int rows = (count + columns - 1) / columns;
    return {columns * (kTileSize + kTileSpacing), rows * (kTileSize + kTileSpacing)};
}

void PinnedSites::rebuild()
{
    while (QLayoutItem* item = m_grid->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    // A single row while the list is short; it starts wrapping when the user
    // pins more than a handful of sites.
    const int columns = qMax(1, qMin(m_sites.size() + 1, 9));
    int index = 0;
    for (const Site& site : std::as_const(m_sites)) {
        auto* tile = new QToolButton(this);
        tile->setObjectName(QStringLiteral("siteTile"));
        tile->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        tile->setIcon(letterAvatar(site.title, site.url));
        tile->setIconSize(QSize(44, 44));
        tile->setText(site.title);
        tile->setToolTip(site.url);
        tile->setCursor(Qt::PointingHandCursor);
        tile->setFixedSize(kTileSize, kTileSize);
        tile->setContextMenuPolicy(Qt::CustomContextMenu);
        const QString url = site.url;
        connect(tile, &QToolButton::clicked, this, [this, url] { emit openUrl(QUrl(url)); });
        connect(tile, &QToolButton::customContextMenuRequested, this, [this, url](const QPoint& at) {
            QMenu menu;
            QAction* remove = menu.addAction(tr("Remove"));
            if (menu.exec(mapToGlobal(at)) == remove) {
                removeSite(url);
            }
        });
        m_grid->addWidget(tile, index / columns, index % columns);
        ++index;
    }

    auto* add = new QToolButton(this);
    add->setObjectName(QStringLiteral("siteTile"));
    add->setToolButtonStyle(Qt::ToolButtonIconOnly);
    add->setIcon(icons::icon(icons::Shape::Plus, 26, QColor(Theme::colors().textMuted)));
    add->setToolTip(tr("Add a site"));
    add->setCursor(Qt::PointingHandCursor);
    add->setFixedSize(kTileSize, kTileSize);
    connect(add, &QToolButton::clicked, this, &PinnedSites::addSite);
    m_grid->addWidget(add, index / columns, index % columns);

    updateGeometry();
}

void PinnedSites::addSite()
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
    save();
}

void PinnedSites::removeSite(const QString& url)
{
    for (int i = 0; i < m_sites.size(); ++i) {
        if (m_sites.at(i).url == url) {
            m_sites.removeAt(i);
            rebuild();
            save();
            return;
        }
    }
}

void PinnedSites::load()
{
    m_sites.clear();

    QJsonArray sites;
    QFile file(AppPaths::pinnedSitesPath());
    if (file.open(QIODevice::ReadOnly)) {
        sites = QJsonDocument::fromJson(file.readAll()).object()
                    .value(QStringLiteral("sites")).toArray();
    }

    bool migrated = false;
    if (sites.isEmpty()) {
        // One-time carry-over from the widget board: the pins the user had
        // there are the one thing worth keeping from it.
        QFile legacy(AppPaths::legacyHomeLayoutPath());
        if (legacy.open(QIODevice::ReadOnly)) {
            const QJsonObject root = QJsonDocument::fromJson(legacy.readAll()).object();
            for (const QJsonValue& value : root.value(QStringLiteral("widgets")).toArray()) {
                const QJsonObject widget = value.toObject();
                if (widget.value(QStringLiteral("type")).toString() == QLatin1String("sites")) {
                    sites = widget.value(QStringLiteral("sites")).toArray();
                    migrated = !sites.isEmpty();
                    break;
                }
            }
        }
    }

    if (sites.isEmpty()) {
        for (const auto& site : defaultSites()) {
            m_sites.append({site.first, site.second});
        }
        rebuild();
        // Written straight away, so the migration above runs once instead of on
        // every start.
        save();
        return;
    }
    for (const QJsonValue& value : std::as_const(sites)) {
        const QJsonObject entry = value.toObject();
        const QString url = entry.value(QStringLiteral("url")).toString();
        if (!url.isEmpty()) {
            m_sites.append({url, entry.value(QStringLiteral("title")).toString()});
        }
    }
    rebuild();

    if (migrated) {
        // Only the pins were carried over, but writing them out means the next
        // start no longer has to open home.json at all.
        save();
    }
}

void PinnedSites::save() const
{
    QJsonArray array;
    for (const Site& site : m_sites) {
        QJsonObject entry;
        entry.insert(QStringLiteral("url"), site.url);
        entry.insert(QStringLiteral("title"), site.title);
        array.append(entry);
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("sites"), array);

    QSaveFile file(AppPaths::pinnedSitesPath());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.commit();
}

// ---------------------------------------------------------------------------
// Home page
// ---------------------------------------------------------------------------

HomePage::HomePage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("homePage"));
    // The sky is painted by this widget, so it must be opaque: leaving the
    // widget translucent makes Qt skip the background fill and hand the paint
    // to a parent that paints the flat window colour instead.
    setAutoFillBackground(false);

    buildLayout();
    m_pins->load();
    m_field->setFocus(Qt::OtherFocusReason);
}

void HomePage::buildLayout()
{
    m_root = new QVBoxLayout(this);
    m_root->setContentsMargins(0, 0, 0, 0);
    m_root->setSpacing(0);
    m_root->addStretch(1);

    m_wordmark = new QLabel(QStringLiteral("✦  YOZORA"), this);
    m_wordmark->setObjectName(QStringLiteral("heroWordmark"));
    m_wordmark->setAlignment(Qt::AlignCenter);
    QFont wordFont(Theme::displayFamily());
    wordFont.setPointSizeF(30.0);
    wordFont.setWeight(QFont::Bold);
    wordFont.setLetterSpacing(QFont::AbsoluteSpacing, 9);
    m_wordmark->setFont(wordFont);

    m_tagline = new QLabel(tr("Your world. Your browser."), this);
    m_tagline->setObjectName(QStringLiteral("heroTagline"));
    m_tagline->setAlignment(Qt::AlignCenter);

    m_field = new GlassField(this);
    m_field->setObjectName(QStringLiteral("heroSearch"));
    m_field->setPlaceholderText(tr("Search the web or enter a URL..."));
    m_field->setHeroMode(true);
    m_field->setRadius(29.0);
    m_field->setTrailing(GlassField::Trailing::Arrow);
    // Room for the shadow to spread: painted at the widget's own bounds it is
    // clipped, and the field ends up looking dented into the sky.
    m_field->setShadowMargin(Glass::shadowMargin());
    m_field->setFixedSize(kFieldWidth + 2 * Glass::shadowMargin(),
                          kFieldHeight + 2 * Glass::shadowMargin());
    m_field->setFocusPolicy(Qt::StrongFocus);

    const auto search = [this] {
        const QString text = m_field->text().trimmed();
        if (!text.isEmpty()) {
            emit searchRequested(text);
        }
    };
    connect(m_field, &GlassField::trailingClicked, this, search);
    connect(m_field, &GlassField::returnPressed, this, search);

    m_pinsTitle = new QLabel(tr("PINNED"), this);
    m_pinsTitle->setObjectName(QStringLiteral("pinsTitle"));
    m_pinsTitle->setAlignment(Qt::AlignCenter);
    QFont pinsFont = m_pinsTitle->font();
    pinsFont.setPointSizeF(9.5);
    pinsFont.setWeight(QFont::DemiBold);
    pinsFont.setLetterSpacing(QFont::AbsoluteSpacing, 3.0);
    m_pinsTitle->setFont(pinsFont);

    m_pins = new PinnedSites(this);
    connect(m_pins, &PinnedSites::openUrl, this, &HomePage::openUrl);

    m_root->addWidget(m_wordmark, 0, Qt::AlignHCenter);
    m_root->addSpacing(6);
    m_root->addWidget(m_tagline, 0, Qt::AlignHCenter);
    m_root->addSpacing(26);
    m_root->addWidget(m_field, 0, Qt::AlignHCenter);
    m_root->addSpacing(40);
    m_root->addWidget(m_pinsTitle, 0, Qt::AlignHCenter);
    m_root->addSpacing(14);
    m_root->addWidget(m_pins, 0, Qt::AlignHCenter);
    m_root->addStretch(2);
}

void HomePage::focusSearch()
{
    m_field->setFocus(Qt::OtherFocusReason);
    m_field->selectAll();
}

int HomePage::topSpacing() const
{
    // Keep the search block in the upper third on a short window, and roughly in
    // the middle on a tall one.
    return qMax(24, static_cast<int>(height() * 0.13));
}

void HomePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_root) {
        m_root->setContentsMargins(0, topSpacing(), 0, 0);
    }
}

void HomePage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    NightSky::paint(painter, size(), devicePixelRatioF());
}

void HomePage::keyPressEvent(QKeyEvent* event)
{
    // Typing anywhere on the page goes to the search field, like a start page
    // should.
    if (event->key() == Qt::Key_Escape) {
        m_field->clear();
        return;
    }
    if (!event->text().isEmpty() && event->text().at(0).isPrint()
        && event->text() != QLatin1String(" ")) {
        m_field->setFocus(Qt::OtherFocusReason);
        m_field->editor()->setText(event->text());
        m_field->editor()->setCursorPosition(event->text().size());
        return;
    }
    QWidget::keyPressEvent(event);
}

}  // namespace yozora
