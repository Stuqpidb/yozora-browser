// SPDX-License-Identifier: MIT
#include "ui/NavigationBar.h"

#include "core/Theme.h"
#include "ui/AddressBar.h"
#include "ui/NavButton.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kMessageTimeoutMs = 2500;
constexpr int kControlHeight = 40;
}  // namespace

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
{
    setObjectName(QStringLiteral("navigationBar"));
    m_layout->setContentsMargins(14, 9, 14, 9);
    m_layout->setSpacing(4);

    m_backButton = makeButton(icons::Shape::Back, tr("Back (Alt+Left)"));
    m_forwardButton = makeButton(icons::Shape::Forward, tr("Forward (Alt+Right)"));
    m_reloadButton = makeButton(icons::Shape::Reload, tr("Reload (Ctrl+R)"));

    connect(m_backButton, &NavButton::clicked, this, &NavigationBar::backRequested);
    connect(m_forwardButton, &NavButton::clicked, this, &NavigationBar::forwardRequested);
    connect(m_reloadButton, &NavButton::clicked, this, [this] {
        if (m_loading) {
            emit stopRequested();
        } else {
            emit reloadRequested();
        }
    });

    m_layout->addSpacing(6);

    // The address field and its progress line live in a stacked container so
    // the progress reads as a thin underline of the field.
    m_addressContainer = new QWidget(this);
    m_addressLayout = new QVBoxLayout(m_addressContainer);
    m_addressLayout->setContentsMargins(0, 0, 0, 0);
    m_addressLayout->setSpacing(3);

    m_addressBar = new AddressBar(m_addressContainer);
    m_addressBar->setMinimumHeight(kControlHeight);
    m_addressBar->setFixedHeight(kControlHeight);
    m_addressLayout->addWidget(m_addressBar);

    m_progress = new QProgressBar(m_addressContainer);
    m_progress->setObjectName(QStringLiteral("pageProgress"));
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(2);
    m_progress->setVisible(false);
    m_progress->setRange(0, 100);
    m_addressLayout->addWidget(m_progress);

    m_layout->addWidget(m_addressContainer, 1);
    m_layout->addSpacing(6);

    m_privateBadge = new QLabel(tr("PRIVATE"), this);
    m_privateBadge->setObjectName(QStringLiteral("privateBadge"));
    m_privateBadge->setVisible(false);
    m_layout->addWidget(m_privateBadge);

    // The shield, in the spirit of Brave: one click shows what was blocked on
    // this page and offers to allow the site.
    m_shieldButton = makeButton(icons::Shape::Shield, tr("Ads and trackers blocked on this page"));
    connect(m_shieldButton, &NavButton::clicked, this, &NavigationBar::shieldRequested);

    m_starButton = makeButton(icons::Shape::Star, tr("Bookmark this page"));
    m_starButton->setCheckable(true);
    connect(m_starButton, &NavButton::clicked, this, &NavigationBar::bookmarkRequested);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("navMessage"));
    m_messageLabel->setVisible(false);
    m_messageLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_layout->addWidget(m_messageLabel);

    m_menuButton = makeButton(icons::Shape::Menu, tr("Menu"));
    connect(m_menuButton, &NavButton::clicked, this, [this] {
        emit menuRequested(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
    });
}

NavigationBar::~NavigationBar() = default;

NavButton* NavigationBar::makeButton(icons::Shape shape, const QString& tooltip)
{
    auto* button = new NavButton(shape, tooltip, this);
    button->setObjectName(QStringLiteral("navButton"));
    m_layout->addWidget(button);
    return button;
}

void NavigationBar::setCanGoBack(bool can)
{
    m_backButton->setEnabled(can);
}

void NavigationBar::setCanGoForward(bool can)
{
    m_forwardButton->setEnabled(can);
}

void NavigationBar::setLoading(bool loading)
{
    m_loading = loading;
    // The same control becomes Stop while the page is loading, so it is
    // always in the same place.
    m_reloadButton->setShape(loading ? icons::Shape::Stop : icons::Shape::Reload);
    m_reloadButton->setToolTip(loading ? tr("Stop (Esc)") : tr("Reload (Ctrl+R)"));
    m_reloadButton->setEnabled(true);
    m_progress->setVisible(loading);
    m_progress->setValue(loading ? 0 : 100);
}

void NavigationBar::setLoadProgress(int percent)
{
    m_progress->setValue(qBound(0, percent, 100));
}

void NavigationBar::setBookmarked(bool bookmarked)
{
    m_starButton->setChecked(bookmarked);
    m_starButton->setToolTip(bookmarked ? tr("Remove bookmark") : tr("Bookmark this page"));
}

void NavigationBar::setPrivateMode(bool enabled)
{
    m_privateBadge->setVisible(enabled);
}

void NavigationBar::setShieldState(int blocked, bool enabled)
{
    // The glyph is accented when the shield is on and something was blocked, so
    // the state is readable at a glance without opening the panel.
    const auto colors = Theme::colors();
    const QColor muted(colors.textMuted);
    const QColor accent(colors.accent);
    m_shieldButton->setIconColor(enabled && blocked > 0 ? accent : muted);
    if (!enabled) {
        m_shieldButton->setToolTip(tr("Tracking protection is off"));
    } else if (blocked > 0) {
        m_shieldButton->setToolTip(tr("%n item(s) blocked on this page", "", blocked));
    } else {
        m_shieldButton->setToolTip(tr("Nothing blocked on this page"));
    }
}

void NavigationBar::showMessage(const QString& message)
{
    showMessage(message, kMessageTimeoutMs);
}

void NavigationBar::showMessage(const QString& message, int timeoutMs)
{
    m_messageLabel->setText(message);
    m_messageLabel->setVisible(!message.isEmpty());
    if (message.isEmpty()) {
        return;
    }
    QTimer::singleShot(timeoutMs, m_messageLabel, [this] { m_messageLabel->setVisible(false); });
}

}  // namespace yozora
