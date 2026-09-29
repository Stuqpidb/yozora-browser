// SPDX-License-Identifier: MIT
#include "ui/NavigationBar.h"

#include "ui/AddressBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace yozora {

namespace {
constexpr int kMessageTimeoutMs = 2500;
constexpr int kAddressBarHeight = 34;
}  // namespace

NavigationBar::NavigationBar(QWidget* parent)
    : QWidget(parent)
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(12, 9, 12, 6);
    m_layout->setSpacing(4);

    m_brandLabel = new QLabel(this);
    m_brandLabel->setObjectName(QStringLiteral("brandLabel"));
    m_brandLabel->setText(QStringLiteral("Yozora"));
    m_layout->addWidget(m_brandLabel);
    m_layout->addSpacing(6);

    m_backButton = makeButton(QStringLiteral("backButton"), QStringLiteral("←"), tr("Back"));
    m_forwardButton =
        makeButton(QStringLiteral("forwardButton"), QStringLiteral("→"), tr("Forward"));
    m_reloadButton =
        makeButton(QStringLiteral("reloadButton"), QStringLiteral("↻"), tr("Reload"));

    connect(m_backButton, &QToolButton::clicked, this, &NavigationBar::backRequested);
    connect(m_forwardButton, &QToolButton::clicked, this, &NavigationBar::forwardRequested);
    connect(m_reloadButton, &QToolButton::clicked, this, [this] {
        if (m_loading) {
            emit stopRequested();
        } else {
            emit reloadRequested();
        }
    });

    // The address field and its progress line live in a stacked container so
    // the progress reads as a thin underline of the field.
    m_addressContainer = new QWidget(this);
    m_addressLayout = new QVBoxLayout(m_addressContainer);
    m_addressLayout->setContentsMargins(0, 0, 0, 0);
    m_addressLayout->setSpacing(2);

    m_addressBar = new AddressBar(m_addressContainer);
    m_addressBar->setObjectName(QStringLiteral("addressBar"));
    m_addressBar->setMinimumHeight(kAddressBarHeight);
    m_addressLayout->addWidget(m_addressBar);

    m_progress = new QProgressBar(m_addressContainer);
    m_progress->setObjectName(QStringLiteral("pageProgress"));
    m_progress->setTextVisible(false);
    m_progress->setFixedHeight(2);
    m_progress->setVisible(false);
    m_progress->setRange(0, 100);
    m_addressLayout->addWidget(m_progress);

    m_layout->addWidget(m_addressContainer, 1);

    m_messageLabel = new QLabel(this);
    m_messageLabel->setObjectName(QStringLiteral("navMessage"));
    m_messageLabel->setVisible(false);
    m_messageLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    m_layout->addWidget(m_messageLabel);
    m_layout->addSpacing(6);

    // The "+" lives at the end of the tab strip, so the navigation bar does not
    // repeat it. Two identical buttons side by side only adds noise.
    m_menuButton = makeButton(QStringLiteral("menuButton"), QStringLiteral("☰"), tr("Menu"));

    connect(m_menuButton, &QToolButton::clicked, this, [this] {
        emit menuRequested(m_menuButton->mapToGlobal(QPoint(0, m_menuButton->height())));
    });
}

NavigationBar::~NavigationBar() = default;

QToolButton* NavigationBar::makeButton(const QString& objectName, const QString& text,
                                       const QString& tooltip)
{
    auto* button = new QToolButton(this);
    button->setObjectName(objectName);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setAutoRaise(true);
    button->setFocusPolicy(Qt::NoFocus);
    button->setCursor(Qt::PointingHandCursor);
    button->setFixedSize(30, kAddressBarHeight);
    m_layout->addWidget(button);
    return button;
}

void NavigationBar::setBrand(const QString& name, const QString& version)
{
    m_brandLabel->setText(QStringLiteral("%1 %2").arg(name, version));
    m_brandLabel->setToolTip(name);
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
    m_reloadButton->setText(loading ? QStringLiteral("✕") : QStringLiteral("↻"));
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
