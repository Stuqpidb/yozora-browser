// SPDX-License-Identifier: MIT
#include "ui/SideBar.h"

#include "ui/RailButton.h"

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

    m_themeButton = new RailButton(RailIcon::Moon, tr("Light theme"), this);
    makeButton(m_themeButton, tr("Light theme"));
    connect(m_themeButton, &RailButton::clicked, this, &SideBar::themeToggleRequested);

    auto* settings = new RailButton(RailIcon::Settings, tr("Settings (Ctrl+,)"), this);
    makeButton(settings, tr("Settings (Ctrl+,)"));
    connect(settings, &RailButton::clicked, this, &SideBar::settingsRequested);
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

void SideBar::setHomeActive(bool active)
{
    m_homeButton->setChecked(active);
}

void SideBar::setDarkTheme(bool dark)
{
    m_themeButton->setIcon(dark ? RailIcon::Sun : RailIcon::Moon);
    m_themeButton->setToolTip(dark ? tr("Light theme") : tr("Dark theme"));
    for (RailButton* button : m_buttons) {
        button->setDarkTheme(dark);
    }
}

}  // namespace yozora
