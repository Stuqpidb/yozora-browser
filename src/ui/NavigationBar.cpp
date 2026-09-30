// SPDX-License-Identifier: MIT
#include "ui/NavigationBar.h"

#include "ui/AddressBar.h"

#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kMessageTimeoutMs = 2500;
constexpr int kControlHeight = 38;
}  // namespace

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(14, 9, 14, 8);
    m_layout->setSpacing(6);

    m_backButton = makeButton(QStringLiteral("navButton"), QStringLiteral("\u2190"),
                              tr("Back (Alt+Left)"));
    m_forwardButton = makeButton(QStringLiteral("navButton"), QStringLiteral("\u2192"),
                                 tr("Forward (Alt+Right)"));
    m_reloadButton = makeButton(QStringLiteral("navButton"), QStringLiteral("\u21BB"),
                                tr("Reload (Ctrl+R)"));

    connect(m_backButton, &QToolButton::clicked, this, &NavigationBar::backRequested);
    connect(m_forwardButton, &QToolButton::clicked, this, &NavigationBar::forwardRequested);
    connect(m_reloadButton, &QToolButton::clicked, this, [this] {
        if (m_loading) {
            emit stopRequested();
        } else {
            emit reloadRequested();
        }
    });

    m_layout->addSpacing(2);

    // The address field and its progress line live in a stacked container so
    // the progress reads as a thin underline of the field.
    m_addressContainer = new QWidget(this);
    m_addressLayout = new QVBoxLayout(m_addressContainer);
    m_addressLayout->setContentsMargins(0, 0, 0, 0);
    m_addressLayout->setSpacing(3);

    m_addressBar = new AddressBar(m_addressContainer);
    m_addressBar->setObjectName(QStringLiteral("addressBar"));
    m_addressBar->setMinimumHeight(kControlHeight);
    m_addressLayout->addWidget(m_addressBar);

    m_progress = new QProgressBar(m_addressContainer);
    m_progress->setObjectName(QStringLiteral("pageProgress"));
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(2);
    m_progress->setVisible(false);
    m_progress->setRange(0, 100);
    m_addressLayout->addWidget(m_progress);

    m_layout->addWidget(m_addressContainer, 1);
    m_layout->addSpacing(2);

    m_privateBadge = new QLabel(tr("PRIVATE"), this);
    m_privateBadge->setObjectName(QStringLiteral("privateBadge"));
    m_privateBadge->setVisible(false);
    m_layout->addWidget(m_privateBadge);

    m_starButton = makeButton(QStringLiteral("navButton"), QStringLiteral("\u2606"),
                              tr("Bookmark this page"));
    m_starButton->setCheckable(true);
    connect(m_starButton, &QToolButton::clicked, this, &NavigationBar::bookmarkRequested);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("navMessage"));
    m_messageLabel->setVisible(false);
    m_messageLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_layout->addWidget(m_messageLabel);

    m_menuButton = makeButton(QStringLiteral("navButton"), QStringLiteral("\u2630"),
                              tr("Menu"));
    connect(m_menuButton, &QToolButton::clicked, this, [this] {
        emit menuRequested(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
    });
}

NavigationBar::~NavigationBar() = default;

QToolButton* NavigationBar::makeButton(const QString& objectName, const QString& glyph,
                                       const QString& tooltip)
{
    auto* button = new QToolButton(this);
    button->setObjectName(objectName);
    button->setText(glyph);
    button->setToolTip(tooltip);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(kControlHeight, kControlHeight);
    QFont font = button->font();
    font.setPointSizeF(15.0);
    button->setFont(font);
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
    m_reloadButton->setText(loading ? QStringLiteral("\u2715") : QStringLiteral("\u21BB"));
    m_reloadButton->setToolTip(loading ? tr("Stop (Esc)") : tr("Reload (Ctrl+R)"));
    m_reloadButton->setEnabled(true);
    m_progress->setVisible(loading);
    if (loading) {
        m_progress->setValue(0);
    } else {
        m_progress->setValue(100);
    }
}

void NavigationBar::setLoadProgress(int percent)
{
    m_progress->setValue(qBound(0, percent, 100));
}

void NavigationBar::setBookmarked(bool bookmarked)
{
    m_starButton->setChecked(bookmarked);
    m_starButton->setText(bookmarked ? QStringLiteral("\u2605") : QStringLiteral("\u2606"));
    m_starButton->setToolTip(bookmarked ? tr("Remove bookmark") : tr("Bookmark this page"));
}

void NavigationBar::setPrivateMode(bool enabled)
{
    m_privateBadge->setVisible(enabled);
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
