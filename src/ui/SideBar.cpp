// SPDX-License-Identifier: MIT
#include "ui/SideBar.h"

#include <QFont>
#include <QToolButton>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kRailWidth = 70;
constexpr int kButtonSize = 50;
}  // namespace

SideBar::SideBar(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    setObjectName(QStringLiteral("sideBar"));
    setFixedWidth(kRailWidth);
    m_layout->setContentsMargins(0, 12, 0, 12);
    m_layout->setSpacing(6);
    m_layout->setAlignment(Qt::AlignHCenter);

    m_homeButton = makeButton(QStringLiteral("\u2302"), tr("Home (Ctrl+Shift+H)"), true);
    connect(m_homeButton, &QToolButton::clicked, this, &SideBar::homeRequested);

    auto* history = makeButton(QStringLiteral("\u27F2"), tr("History"));
    connect(history, &QToolButton::clicked, this, &SideBar::historyRequested);

    auto* bookmarks = makeButton(QStringLiteral("\u2691"), tr("Bookmarks"));
    connect(bookmarks, &QToolButton::clicked, this, &SideBar::bookmarksRequested);

    auto* downloads = makeButton(QStringLiteral("\u21E9"), tr("Downloads"));
    connect(downloads, &QToolButton::clicked, this, &SideBar::downloadsRequested);

    auto* profile = makeButton(QStringLiteral("\u25D0"), tr("New private window"));
    connect(profile, &QToolButton::clicked, this, &SideBar::privateRequested);

    m_layout->addStretch(1);

    m_themeButton = makeButton(QStringLiteral("\u263E"), tr("Light theme"));
    connect(m_themeButton, &QToolButton::clicked, this, &SideBar::themeToggleRequested);

    auto* settings = makeButton(QStringLiteral("\u2699"), tr("Settings (Ctrl+,)"));
    connect(settings, &QToolButton::clicked, this, &SideBar::settingsRequested);
}

QToolButton* SideBar::makeButton(const QString& glyph, const QString& tooltip, bool checkable)
{
    auto* button = new QToolButton(this);
    button->setObjectName(QStringLiteral("railButton"));
    button->setText(glyph);
    button->setToolTip(tooltip);
    button->setCheckable(checkable);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(kButtonSize, kButtonSize);
    QFont font = button->font();
    font.setPointSizeF(21.0);
    button->setFont(font);
    m_layout->addWidget(button, 0, Qt::AlignHCenter);
    return button;
}

void SideBar::setHomeActive(bool active)
{
    m_homeButton->setChecked(active);
}

void SideBar::setDarkTheme(bool dark)
{
    m_themeButton->setToolTip(dark ? tr("Light theme") : tr("Dark theme"));
    m_themeButton->setText(dark ? QStringLiteral("\u2600") : QStringLiteral("\u263E"));
}

}  // namespace yozora
