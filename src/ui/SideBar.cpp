// SPDX-License-Identifier: MIT
#include "ui/SideBar.h"

#include "ui/RailButton.h"

#include "core/Animation.h"

#include <QPropertyAnimation>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kRailWidth = 72;
constexpr int kButtonSize = 48;

// The rail is animated to zero rather than to a narrow strip: a 12px column of
// half-visible icons looks broken, whereas no rail at all is obviously "hidden".
constexpr int kCollapsedWidth = 0;
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

    // The collapse toggle lives in the rail, at the bottom with the settings
    // button, and points at the edge the rail slides into.
    m_collapseButton = new RailButton(RailIcon::Collapse, tr("Hide the sidebar"), this);
    makeButton(m_collapseButton, tr("Hide the sidebar"));
    connect(m_collapseButton, &RailButton::clicked, this, &SideBar::onCollapseToggled);

    auto* settings = new RailButton(RailIcon::Settings, tr("Settings (Ctrl+,)"), this);
    makeButton(settings, tr("Settings (Ctrl+,)"));
    connect(settings, &RailButton::clicked, this, &SideBar::settingsRequested);

    m_widthAnimation = new QPropertyAnimation(this, "minimumWidth", this);
    Animation::configure(m_widthAnimation, Animation::kStandardMs);
    connect(m_widthAnimation, &QVariantAnimation::valueChanged, this, [this] {
        setMaximumWidth(width());
    });
    connect(m_widthAnimation, &QVariantAnimation::finished, this, [this] {
        // Only hide once the slide is over, otherwise the buttons vanish before
        // the rail has finished moving.
        setVisible(!m_collapsed);
        if (m_collapsed) {
            setMinimumWidth(0);
            setMaximumWidth(kCollapsedWidth);
        }
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

void SideBar::onCollapseToggled()
{
    setCollapsed(!m_collapsed);
}

void SideBar::setCollapsed(bool collapsed, bool animate)
{
    if (m_collapsed == collapsed) {
        return;
    }
    m_collapsed = collapsed;
    m_collapseButton->setToolTip(collapsed ? tr("Show the sidebar") : tr("Hide the sidebar"));

    const int target = collapsed ? kCollapsedWidth : kRailWidth;

    if (!animate || !isVisible()) {
        // Without an animation (or before the window is shown) the state is set
        // outright, otherwise a rail that is already hidden would animate from
        // whatever width it happened to have.
        m_widthAnimation->stop();
        setVisible(!collapsed);
        setMinimumWidth(target);
        setMaximumWidth(target);
        emit collapsedChanged(m_collapsed);
        return;
    }

    setVisible(true);
    Animation::start(m_widthAnimation, width(), target);
    emit collapsedChanged(m_collapsed);
}

}  // namespace yozora
