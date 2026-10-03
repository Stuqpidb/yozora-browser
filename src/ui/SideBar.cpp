// SPDX-License-Identifier: MIT
#include "ui/SideBar.h"

#include "ui/RailButton.h"

#include "core/Animation.h"

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kRailWidth = 72;
constexpr int kButtonSize = 48;
}  // namespace

SideBar::SideBar(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    setObjectName(QStringLiteral("sideBar"));
    setFixedWidth(kRailWidth);
    m_layout->setContentsMargins(0, 14, 0, 14);
    m_layout->setSpacing(6);
    m_layout->setAlignment(Qt::AlignHCenter);

    m_homeButton = new RailButton(RailIcon::Home, tr("Home (Ctrl+Shift+H)"), this);
    makeButton(m_homeButton, tr("Home (Ctrl+Shift+H)"));
    connect(m_homeButton, &RailButton::clicked, this, &SideBar::homeRequested);

    auto* history = new RailButton(RailIcon::History, tr("History"), this);
    makeButton(history, tr("History"));
    connect(history, &RailButton::clicked, this, &SideBar::historyRequested);

    auto* bookmarks = new RailButton(RailIcon::Bookmarks, tr("Bookmarks"), this);
    makeButton(bookmarks, tr("Bookmarks"));
    connect(bookmarks, &RailButton::clicked, this, &SideBar::bookmarksRequested);

    auto* downloads = new RailButton(RailIcon::Downloads, tr("Downloads"), this);
    makeButton(downloads, tr("Downloads"));
    connect(downloads, &RailButton::clicked, this, &SideBar::downloadsRequested);

    auto* profile = new RailButton(RailIcon::Private, tr("New private window"), this);
    makeButton(profile, tr("New private window"));
    connect(profile, &RailButton::clicked, this, &SideBar::privateRequested);

    m_layout->addStretch(1);

    auto* settings = new RailButton(RailIcon::Settings, tr("Settings (Ctrl+,)"), this);
    makeButton(settings, tr("Settings (Ctrl+,)"));
    connect(settings, &RailButton::clicked, this, &SideBar::settingsRequested);

    // The rail is an overlay: it slides off to the left instead of shrinking,
    // so the page behind it never reflows (that reflow read as the whole window
    // collapsing). The animation drives the widget's x position.
    m_slideAnimation = new QPropertyAnimation(this, "pos", this);
    Animation::configure(m_slideAnimation, Animation::kStandardMs);

    m_opacity = new QGraphicsOpacityEffect(this);
    m_opacity->setOpacity(1.0);
    setGraphicsEffect(m_opacity);
    m_opacityAnimation = new QVariantAnimation(this);
    Animation::configure(m_opacityAnimation, Animation::kStandardMs);
    connect(m_opacityAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& v) {
        m_opacity->setOpacity(v.toReal());
    });
}

RailButton* SideBar::makeButton(RailButton* button, const QString& tooltip)
{
    button->setObjectName(QStringLiteral("railButton"));
    button->setToolTip(tooltip);
    button->setFixedSize(kButtonSize, kButtonSize);
    m_layout->addWidget(button, 0, Qt::AlignHCenter);
    m_buttons.append(button);
    return button;
}

int SideBar::expandedWidth() const
{
    return kRailWidth;
}

void SideBar::setHomeActive(bool active)
{
    m_homeButton->setCurrent(active);
}

void SideBar::setAvailableHeight(int height)
{
    resize(kRailWidth, height);
    // The slide animation owns x while it runs; otherwise place the rail at its
    // resting x for the current state.
    if (m_slideAnimation->state() != QAbstractAnimation::Running) {
        move(m_collapsed ? -kRailWidth : 0, 0);
    }
}

void SideBar::setCollapsed(bool collapsed, bool animate)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;

    const int targetX = collapsed ? -kRailWidth : 0;
    const qreal targetOpacity = collapsed ? 0.0 : 1.0;

    if (!animate || !isVisible()) {
        // Before the window is shown there is nothing to animate.
        m_slideAnimation->stop();
        m_opacityAnimation->stop();
        move(targetX, y());
        m_opacity->setOpacity(targetOpacity);
        emit collapsedChanged(m_collapsed);
        return;
    }

    m_slideAnimation->stop();
    m_slideAnimation->setStartValue(QPoint(x(), y()));
    m_slideAnimation->setEndValue(QPoint(targetX, y()));
    m_slideAnimation->start();
    Animation::start(m_opacityAnimation, m_opacity->opacity(), targetOpacity);
    emit collapsedChanged(m_collapsed);
}

}  // namespace yozora
