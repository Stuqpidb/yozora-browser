// SPDX-License-Identifier: MIT
#include "home/HomeWidget.h"

#include "core/Glass.h"
#include "core/Theme.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QLinearGradient>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QToolButton>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kHeaderHeight = 36;
constexpr int kResizeCorner = 22;
constexpr int kMinCardWidth = 200;
constexpr int kMinCardHeight = 140;
}  // namespace

HomeWidget::HomeWidget(const HomeContext& context, QWidget* parent)
    : QFrame(parent)
    , m_context(context)
{
    setObjectName(QStringLiteral("homeCard"));
    // The body is painted in paintEvent(); the style sheet only styles the
    // header, the labels and the controls inside the card.
    setAttribute(Qt::WA_StyledBackground, false);
    setMouseTracking(true);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 10, 18, 16);
    root->setSpacing(10);

    auto* header = new QHBoxLayout;
    header->setContentsMargins(0, 0, 0, 0);
    header->setSpacing(8);
    m_header = header;

    m_handle = new QLabel(QStringLiteral("\u28FF"), this);
    m_handle->setObjectName(QStringLiteral("cardSubtle"));
    m_handle->setToolTip(tr("Drag to move"));
    header->addWidget(m_handle);

    m_title = new QLabel(this);
    m_title->setObjectName(QStringLiteral("cardTitle"));
    header->addWidget(m_title);
    header->addStretch(1);

    m_menuButton = new QToolButton(this);
    m_menuButton->setObjectName(QStringLiteral("cardMenu"));
    m_menuButton->setText(QStringLiteral("\u22EF"));
    m_menuButton->setToolTip(tr("Widget options"));
    m_menuButton->setCursor(Qt::PointingHandCursor);
    m_menuButton->setFixedSize(26, 26);
    m_menuButton->setFocusPolicy(Qt::NoFocus);
    connect(m_menuButton, &QToolButton::clicked, this, [this] {
        showCardMenu(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
    });
    header->addWidget(m_menuButton);

    root->addLayout(header);

    m_body = new QWidget(this);
    auto* bodyLayout = new QVBoxLayout(m_body);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(8);
    root->addWidget(m_body, 1);

    resize(m_cardSize);
}

void HomeWidget::setTitle(const QString& title)
{
    m_title->setText(title);
}

QVBoxLayout* HomeWidget::bodyLayout() const
{
    return qobject_cast<QVBoxLayout*>(m_body->layout());
}

void HomeWidget::setHeaderVisible(bool visible)
{
    m_handle->setVisible(visible);
    m_title->setVisible(visible);
    m_menuButton->setVisible(visible);
}

void HomeWidget::addHeaderAction(QWidget* widget)
{
    if (!m_header || !widget) {
        return;
    }
    const int index = m_header->indexOf(m_menuButton);
    m_header->insertWidget(index < 0 ? m_header->count() : index, widget);
}

void HomeWidget::setCardSize(const QSize& size)
{
    QSize wanted = size;
    if (!wanted.isValid()) {
        wanted = QSize(360, 260);
    }
    wanted = wanted.expandedTo(QSize(kMinCardWidth, kMinCardHeight));
    m_cardSize = wanted;
    resize(wanted);
}

void HomeWidget::setFixed(bool fixed)
{
    m_fixed = fixed;
    setHeaderVisible(!fixed);
}

QRect HomeWidget::headerRect() const
{
    return QRect(0, 0, width(), kHeaderHeight);
}

bool HomeWidget::isOverResizeCorner(const QPoint& pos) const
{
    if (m_fixed) {
        return false;
    }
    return pos.x() >= width() - kResizeCorner && pos.y() >= height() - kResizeCorner;
}

void HomeWidget::showCardMenu(const QPoint& globalPos)
{
    QMenu menu(this);
    QAction* resetSize = menu.addAction(tr("Reset size"));
    connect(resetSize, &QAction::triggered, this,
            [this] { setCardSize(QSize(360, 260)); emit changed(); });
    menu.addSeparator();
    QAction* remove = menu.addAction(tr("Remove widget"));
    connect(remove, &QAction::triggered, this, &HomeWidget::removeRequested);
    menu.exec(globalPos);
}

void HomeWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    // The search hero floats directly on the night sky, so it must paint
    // absolutely nothing: not even a transparent clear, because clearing the
    // backing store with CompositionMode_Source leaves an opaque black patch
    // under a widget whose parent is only a plain QWidget.
    if (m_fixed) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // The rounded corners need clean pixels, so clear to transparent first.
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const bool dark = Theme::isDark();
    const auto colors = dark ? Theme::darkColors() : Theme::lightColors();
    Glass::Recipe glass = Glass::recipe(dark, 20);

    if (m_dragging || property("dropTarget").toBool()) {
        glass.stroke = QColor(colors.accent);
        glass.highlight = QColor(colors.accent);
    }

    const QRectF body = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    Glass::paintShadow(painter, body, glass, 1.0);
    Glass::paintPanel(painter, body, glass, 1.0);
}

void HomeWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QFrame::mousePressEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();

    if (isOverResizeCorner(pos)) {
        m_resizing = true;
        m_pressOffset = pos;
        m_startSize = size();
        event->accept();
        return;
    }
    if (!m_fixed && headerRect().contains(pos)) {
        m_dragging = true;
        m_pressOffset = pos;
        setCursor(Qt::ClosedHandCursor);
        emit dragBegin();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void HomeWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();

    if (m_resizing) {
        const QSize next(m_startSize.width() + (pos.x() - m_pressOffset.x()),
                         m_startSize.height() + (pos.y() - m_pressOffset.y()));
        resize(next.expandedTo(QSize(kMinCardWidth, kMinCardHeight)));
        event->accept();
        return;
    }
    if (m_dragging) {
        emit dragMove(event->globalPosition().toPoint());
        event->accept();
        return;
    }

    // Hover feedback: a grab hand over the header, a diagonal arrow over the
    // resize corner, and a normal cursor everywhere else.
    if (!m_fixed) {
        const bool overHeader = headerRect().contains(pos);
        if (isOverResizeCorner(pos)) {
            setCursor(Qt::SizeFDiagCursor);
        } else if (overHeader) {
            setCursor(Qt::OpenHandCursor);
        } else {
            unsetCursor();
        }
    }
    QFrame::mouseMoveEvent(event);
}

void HomeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (m_resizing) {
        m_resizing = false;
        m_cardSize = size();
        emit changed();
        event->accept();
        return;
    }
    if (m_dragging) {
        m_dragging = false;
        emit dragDrop(event->globalPosition().toPoint());
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

QJsonObject HomeWidget::save() const
{
    QJsonObject object;
    object.insert(QStringLiteral("type"), typeId());
    object.insert(QStringLiteral("width"), m_cardSize.width());
    object.insert(QStringLiteral("height"), m_cardSize.height());
    object.insert(QStringLiteral("settings"), saveSettings());
    return object;
}

void HomeWidget::restore(const QJsonObject& object)
{
    setCardSize(QSize(object.value(QStringLiteral("width")).toInt(m_cardSize.width()),
                      object.value(QStringLiteral("height")).toInt(m_cardSize.height())));
    loadSettings(object.value(QStringLiteral("settings")).toObject());
}

}  // namespace yozora
