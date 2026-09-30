// SPDX-License-Identifier: MIT
#include "home/HomePage.h"

#include "app/AppPaths.h"
#include "home/HomeWidgets.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLinearGradient>
#include <QMenu>
#include <QPainter>
#include <QRandomGenerator>
#include <QResizeEvent>
#include <QSaveFile>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace yozora {

namespace {

void paintNightSky(QPainter& painter, const QRect& rect, bool dark)
{
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    if (dark) {
        gradient.setColorAt(0.0, QColor(0x14, 0x1b, 0x3c));
        gradient.setColorAt(0.45, QColor(0x1b, 0x16, 0x46));
        gradient.setColorAt(1.0, QColor(0x0c, 0x10, 0x2c));
    } else {
        gradient.setColorAt(0.0, QColor(0xf6, 0xf8, 0xfe));
        gradient.setColorAt(1.0, QColor(0xe4, 0xea, 0xf8));
    }
    painter.fillRect(rect, gradient);

    const auto nebula = [&painter, &rect](qreal cx, qreal cy, qreal spread, const QColor& color) {
        const qreal radius = qMax(rect.width(), rect.height()) * spread;
        QRadialGradient glow(QPointF(rect.width() * cx, rect.height() * cy), radius);
        glow.setColorAt(0.0, color);
        glow.setColorAt(1.0, QColor(color.red(), color.green(), color.blue(), 0));
        painter.setPen(Qt::NoPen);
        painter.setBrush(glow);
        painter.drawEllipse(QPointF(rect.width() * cx, rect.height() * cy), radius, radius);
    };
    nebula(0.18, 0.12, 0.45, dark ? QColor(0x6a, 0x3c, 0xff, 60) : QColor(0x9a, 0x8c, 0xff, 40));
    nebula(0.85, 0.30, 0.40, dark ? QColor(0x2f, 0x7b, 0xff, 55) : QColor(0x7f, 0xb0, 0xff, 36));

    QRandomGenerator random(0x59A0);
    const int starCount = qBound(70, rect.width() * rect.height() / 6500, 340);
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < starCount; ++i) {
        const int x = static_cast<int>(random.bounded(static_cast<quint32>(qMax(1, rect.width()))));
        const int y = static_cast<int>(random.bounded(static_cast<quint32>(qMax(1, rect.height()))));
        const int alpha = 70 + static_cast<int>(random.bounded(165));
        const int size = (i % 40 == 0) ? 2 : 1;
        painter.setBrush(QColor(0xd6, 0xe0, 0xff, dark ? alpha : alpha / 4));
        painter.drawEllipse(x, y, size, size);
    }

    // A moon with a soft halo.
    const QPointF moon(rect.width() * 0.80, rect.height() * 0.18);
    const qreal halo = qMax(rect.width(), rect.height()) * 0.20;
    QRadialGradient moonGlow(moon, halo);
    moonGlow.setColorAt(0.0, QColor(0xdf, 0xe6, 0xff, dark ? 90 : 70));
    moonGlow.setColorAt(0.25, QColor(0x9a, 0xa8, 0xff, dark ? 40 : 30));
    moonGlow.setColorAt(1.0, QColor(0x6e, 0x8c, 0xff, 0));
    painter.setBrush(moonGlow);
    painter.drawEllipse(moon, halo, halo);
    if (dark) {
        painter.setBrush(QColor(0xea, 0xee, 0xff, 230));
        painter.drawEllipse(moon, 26, 26);
        painter.setBrush(QColor(0xd2, 0xd8, 0xf2, 220));
        painter.drawEllipse(QPointF(moon.x() + 9, moon.y() - 7), 7, 7);
    }
}

}  // namespace

HomePage::HomePage(const HomeContext& context, QWidget* parent)
    : QWidget(parent)
    , m_context(context)
{
    setObjectName(QStringLiteral("homePage"));

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // The hero is pinned like the Chrome start page and lives above the canvas.
    m_hero = new SearchWidget(m_context, this);
    m_hero->setFixed(true);
    connect(m_hero, &SearchWidget::searchRequested, this, &HomePage::searchRequested);
    root->addWidget(m_hero);

    // The canvas holds every movable widget, absolutely positioned.
    m_scroll = new QScrollArea(this);
    m_scroll->setObjectName(QStringLiteral("homeScroll"));
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scroll->setWidgetResizable(false);
    m_scroll->viewport()->setAutoFillBackground(false);
    m_canvas = new QWidget(m_scroll);
    m_canvas->setAttribute(Qt::WA_StyledBackground, false);
    m_scroll->setWidget(m_canvas);
    m_scroll->viewport()->installEventFilter(this);
    root->addWidget(m_scroll, 1);

    m_addButton = new QToolButton(this);
    m_addButton->setObjectName(QStringLiteral("navButton"));
    m_addButton->setText(QStringLiteral("+"));
    m_addButton->setToolTip(tr("Add a widget"));
    m_addButton->setCursor(Qt::PointingHandCursor);
    m_addButton->setFixedSize(40, 40);
    QFont plusFont = m_addButton->font();
    plusFont.setPointSizeF(18);
    m_addButton->setFont(plusFont);
    m_addButton->raise();
    connect(m_addButton, &QToolButton::clicked, this, &HomePage::showAddMenu);

    m_saveTimer = new QTimer(this);
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(400);
    connect(m_saveTimer, &QTimer::timeout, this, &HomePage::saveLayout);

    loadLayout();
    layoutCanvas();
}

HomePage::~HomePage() = default;

HomeWidget* HomePage::createWidget(const QString& type)
{
    HomeWidget* widget = nullptr;
    if (type == QLatin1String("sites")) {
        auto* sites = new SitesWidget(m_context, m_canvas);
        connect(sites, &SitesWidget::openUrl, this, &HomePage::openUrl);
        widget = sites;
    } else if (type == QLatin1String("bookmarks")) {
        auto* bookmarks = new BookmarksWidget(m_context, m_canvas);
        connect(bookmarks, &BookmarksWidget::openUrl, this, &HomePage::openUrl);
        widget = bookmarks;
    } else if (type == QLatin1String("history")) {
        auto* history = new HistoryWidget(m_context, m_canvas);
        connect(history, &HistoryWidget::openUrl, this, &HomePage::openUrl);
        widget = history;
    } else if (type == QLatin1String("quickaccess")) {
        auto* quick = new QuickAccessWidget(m_context, m_canvas);
        connect(quick, &QuickAccessWidget::openUrl, this, &HomePage::openUrl);
        widget = quick;
    } else if (type == QLatin1String("stats")) {
        widget = new StatsWidget(m_context, m_canvas);
    } else if (type == QLatin1String("privacy")) {
        widget = new PrivacyWidget(m_context, m_canvas);
    } else if (type == QLatin1String("notes")) {
        widget = new NotesWidget(m_context, m_canvas);
    } else if (type == QLatin1String("weather")) {
        widget = new WeatherWidget(m_context, m_canvas);
    } else if (type == QLatin1String("clock")) {
        widget = new ClockWidget(m_context, m_canvas);
    }

    if (widget) {
        connect(widget, &HomeWidget::removeRequested, this, [this, widget] { removeWidget(widget); });
        connect(widget, &HomeWidget::changed, this, &HomePage::onWidgetChanged);
        connect(widget, &HomeWidget::dragBegin, this, [this, widget] { beginDrag(widget); });
        connect(widget, &HomeWidget::dragMove, this, &HomePage::updateDrag);
        connect(widget, &HomeWidget::dragDrop, this, [this](const QPoint&) { finishDrag(); });
    }
    return widget;
}

void HomePage::placeWidget(HomeWidget* widget, const Placement& placement, bool)
{
    m_widgets.append(widget);
    m_placements.insert(widget, placement);
}

void HomePage::addDefaultWidget(const QString& type, Placement placement)
{
    if (HomeWidget* widget = createWidget(type)) {
        placeWidget(widget, placement, false);
    }
}

void HomePage::buildDefaultLayout()
{
    for (HomeWidget* widget : m_widgets) {
        widget->deleteLater();
    }
    m_widgets.clear();
    m_placements.clear();

    addDefaultWidget(QStringLiteral("sites"), Placement{0.0, 0, 1.0, 310});
    addDefaultWidget(QStringLiteral("quickaccess"), Placement{0.0, 322, 1.0, 92});
    addDefaultWidget(QStringLiteral("history"), Placement{0.0, 426, 0.485, 260});
    addDefaultWidget(QStringLiteral("bookmarks"), Placement{0.515, 426, 0.485, 260});
    addDefaultWidget(QStringLiteral("stats"), Placement{0.0, 698, 0.315, 250});
    addDefaultWidget(QStringLiteral("privacy"), Placement{0.3425, 698, 0.315, 250});
    addDefaultWidget(QStringLiteral("weather"), Placement{0.6725, 698, 0.3275, 250});
    addDefaultWidget(QStringLiteral("notes"), Placement{0.0, 956, 0.485, 240});
    addDefaultWidget(QStringLiteral("clock"), Placement{0.515, 956, 0.485, 240});

    scheduleSave();
}

void HomePage::loadLayout()
{
    QFile file(AppPaths::homeLayoutPath());
    if (!file.open(QIODevice::ReadOnly)) {
        buildDefaultLayout();
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray array = document.isArray()
        ? document.array()
        : document.object().value(QStringLiteral("widgets")).toArray();
    if (array.isEmpty()) {
        buildDefaultLayout();
        return;
    }
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        HomeWidget* widget = createWidget(object.value(QStringLiteral("type")).toString());
        if (!widget) {
            continue;
        }
        widget->restore(object);
        Placement placement;
        placement.x = qBound(0.0, object.value(QStringLiteral("x")).toDouble(), 1.0);
        placement.y = qMax(0, object.value(QStringLiteral("y")).toInt());
        placement.w = qBound(0.05, object.value(QStringLiteral("w")).toDouble(), 1.0);
        placement.h = qMax(120, object.value(QStringLiteral("h")).toInt());
        placeWidget(widget, placement, true);
    }
}

void HomePage::layoutCanvas()
{
    const int width = qMax(1, m_scroll->viewport()->width() - 52);
    int bottom = 0;
    for (HomeWidget* widget : m_widgets) {
        const Placement placement = m_placements.value(widget);
        const int x = 26 + qRound(placement.x * width);
        const int w = qRound(placement.w * width);
        widget->setGeometry(x, placement.y, w, placement.h);
        bottom = qMax(bottom, placement.y + placement.h);
    }
    const int canvasHeight = qMax(bottom + 28, m_scroll->viewport()->height());
    m_canvas->setGeometry(26, 0, width, canvasHeight);
}

void HomePage::onWidgetChanged()
{
    // Size changes are stored with the widget itself; the canvas only needs
    // to recompute its own height so the page can scroll.
    scheduleSave();
    layoutCanvas();
}

void HomePage::addWidgetOfType(const QString& type)
{
    HomeWidget* widget = createWidget(type);
    if (!widget) {
        return;
    }
    // Put the new card below everything that is already on the page.
    int bottom = 0;
    for (HomeWidget* other : m_widgets) {
        const Placement placement = m_placements.value(other);
        bottom = qMax(bottom, placement.y + placement.h);
    }
    placeWidget(widget, Placement{0.0, bottom + 20, 0.485, 260}, true);
    layoutCanvas();
    widget->raise();
    scheduleSave();
}

void HomePage::removeWidget(HomeWidget* widget)
{
    m_placements.remove(widget);
    m_widgets.removeAll(widget);
    widget->deleteLater();
    layoutCanvas();
    scheduleSave();
}

void HomePage::scheduleSave()
{
    m_saveTimer->start();
}

void HomePage::saveLayout() const
{
    AppPaths::ensureCreated();
    QJsonArray array;
    for (HomeWidget* widget : m_widgets) {
        const Placement placement = m_placements.value(widget);
        QJsonObject object = widget->save();
        object.insert(QStringLiteral("x"), placement.x);
        object.insert(QStringLiteral("y"), placement.y);
        object.insert(QStringLiteral("w"), placement.w);
        object.insert(QStringLiteral("h"), placement.h);
        array.append(object);
    }
    QSaveFile file(AppPaths::homeLayoutPath());
    if (!file.open(QIODevice::WriteOnly)) {
        return;
    }
    file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
    file.commit();
}

void HomePage::showAddMenu()
{
    QMenu menu(this);
    struct Entry {
        const char* type;
        QString label;
    };
    const QList<Entry> entries = {
        {"sites", tr("Sites")},
        {"bookmarks", tr("Bookmarks")}, {"history", tr("History")},
        {"stats", tr("Quick stats")},   {"privacy", tr("Privacy")},
        {"notes", tr("Notes")},         {"weather", tr("Weather")},
        {"clock", tr("Time")},          {"quickaccess", tr("Quick access")},
    };
    for (const Entry& entry : entries) {
        QAction* action = menu.addAction(entry.label);
        const QString type = QString::fromLatin1(entry.type);
        connect(action, &QAction::triggered, this, [this, type] { addWidgetOfType(type); });
    }
    menu.exec(m_addButton->mapToGlobal(QPoint(0, m_addButton->height())));
}

// A grab point of the card when a drag starts, so the widget never "jumps".
void HomePage::beginDrag(HomeWidget* widget)
{
    m_dragWidget = widget;
    const QPoint now = m_canvas->mapFromGlobal(QCursor::pos());
    m_dragOffset = now - widget->geometry().topLeft();
    widget->raise();
}

void HomePage::updateDrag(const QPoint& globalPos)
{
    if (!m_dragWidget) {
        return;
    }
    const QPoint pos = m_canvas->mapFromGlobal(globalPos);
    QPoint target = pos - m_dragOffset;
    // Snap to a 8px grid so the arrangement stays tidy, then stay inside the
    // canvas.
    target.setX(qRound(qreal(target.x()) / 8.0) * 8);
    target.setY(qRound(target.y() / 8.0) * 8);
    const int maxX = qMax(0, m_canvas->width() - m_dragWidget->width());
    const int maxY = qMax(0, m_canvas->height() - m_dragWidget->height());
    target.setX(qBound(0, target.x(), maxX));
    target.setY(qBound(0, target.y(), maxY));
    m_dragWidget->move(target);
}

void HomePage::finishDrag()
{
    if (m_dragWidget) {
        const Placement previous = m_placements.value(m_dragWidget);
        Placement placed = previous;
        placed.x = static_cast<qreal>(m_dragWidget->geometry().x()) / qMax(1, m_canvas->width());
        placed.w = static_cast<qreal>(m_dragWidget->geometry().width()) / qMax(1, m_canvas->width());
        placed.y = m_dragWidget->geometry().y();
        placed.h = m_dragWidget->geometry().height();
        m_placements.insert(m_dragWidget, placed);
        scheduleSave();
    }
    m_dragWidget = nullptr;
}

void HomePage::focusSearch()
{
    if (m_hero) {
        m_hero->focusField();
    }
}

void HomePage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    const bool dark = palette().color(QPalette::Window).lightness() < 128;
    paintNightSky(painter, rect(), dark);
}

void HomePage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (m_addButton) {
        m_addButton->move(width() - m_addButton->width() - 26, 30);
    }
    layoutCanvas();
}

bool HomePage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_scroll->viewport() && event->type() == QEvent::Resize) {
        layoutCanvas();
    }
    return QWidget::eventFilter(watched, event);
}

}  // namespace yozora
