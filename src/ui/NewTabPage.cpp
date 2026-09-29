// SPDX-License-Identifier: MIT
#include "ui/NewTabPage.h"

#include "core/Theme.h"
#include "utils/Version.h"

#include <QFont>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QRandomGenerator>
#include <QVBoxLayout>

namespace yozora {

namespace {

// A fixed star field. The seed is constant so the sky does not shimmer between
// repaints or between windows.
void paintSky(QPainter& painter, const QRect& rect, bool dark)
{
    QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
    if (dark) {
        gradient.setColorAt(0.0, QColor(0x0a, 0x0e, 0x16));
        gradient.setColorAt(0.5, QColor(0x0d, 0x10, 0x17));
        gradient.setColorAt(1.0, QColor(0x12, 0x17, 0x24));
    } else {
        gradient.setColorAt(0.0, QColor(0xf8, 0xfa, 0xfd));
        gradient.setColorAt(1.0, QColor(0xe6, 0xeb, 0xf4));
    }
    painter.fillRect(rect, gradient);

    QRandomGenerator random(0x59A0);  // "Yozora"
    const int area = rect.width() * rect.height();
    const int starCount = qBound(40, area / 9000, 220);

    painter.setPen(Qt::NoPen);
    for (int i = 0; i < starCount; ++i) {
        const int x = static_cast<int>(random.bounded(rect.width()));
        const int y = static_cast<int>(random.bounded(rect.height()));
        const int alpha = 70 + static_cast<int>(random.bounded(150));
        painter.setBrush(QColor(0xc8, 0xd6, 0xf5, dark ? alpha : alpha / 3));
        painter.drawEllipse(x, y, 1, 1);
    }

    // A single soft glow, like a moon behind haze.
    const qreal radius = qMax(rect.width(), rect.height()) * 0.45;
    const QPointF center(rect.width() * 0.76, rect.height() * 0.20);
    QRadialGradient glow(center, radius);
    glow.setColorAt(0.0, QColor(0x6e, 0xa8, 0xfe, dark ? 28 : 34));
    glow.setColorAt(1.0, QColor(0x6e, 0xa8, 0xfe, 0));
    painter.setBrush(glow);
    painter.drawEllipse(center, radius, radius);
}

QFont spacedFont(const QString& family, int pointSize, qreal letterSpacing, int weight)
{
    QFont font(family, pointSize, weight);
    font.setLetterSpacing(QFont::AbsoluteSpacing, letterSpacing);
    return font;
}

}  // namespace

NewTabPage::NewTabPage(QWidget* parent)
    : QWidget(parent)
{
    setObjectName(QStringLiteral("newTabPage"));

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addStretch(2);

    m_wordmark = new QLabel(QStringLiteral("Yozora"), this);
    m_wordmark->setObjectName(QStringLiteral("newTabWordmark"));
    m_wordmark->setAlignment(Qt::AlignCenter);

    m_tagline = new QLabel(tr("NIGHT SKY"), this);
    m_tagline->setObjectName(QStringLiteral("newTabTagline"));
    m_tagline->setAlignment(Qt::AlignCenter);

    m_searchField = new QLineEdit(this);
    m_searchField->setObjectName(QStringLiteral("newTabSearch"));
    m_searchField->setPlaceholderText(tr("Search the web"));
    m_searchField->setClearButtonEnabled(true);
    m_searchField->setFixedSize(460, 46);

    m_searchButton = new QPushButton(tr("Search"), this);
    m_searchButton->setObjectName(QStringLiteral("newTabSearchButton"));
    m_searchButton->setFixedSize(300, 44);
    m_searchButton->setCursor(Qt::PointingHandCursor);
    m_searchButton->setFocusPolicy(Qt::NoFocus);

    m_hint = new QLabel(tr("Enter an address or a search"), this);
    m_hint->setObjectName(QStringLiteral("newTabHint"));
    m_hint->setAlignment(Qt::AlignCenter);

    m_version = new QLabel(this);
    m_version->setObjectName(QStringLiteral("newTabVersion"));
    m_version->setAlignment(Qt::AlignCenter);

    auto* center = new QWidget(this);
    center->setObjectName(QStringLiteral("newTabCenter"));
    auto* centerLayout = new QVBoxLayout(center);
    centerLayout->setContentsMargins(0, 0, 0, 0);
    centerLayout->setSpacing(0);
    centerLayout->setAlignment(Qt::AlignCenter);
    centerLayout->addWidget(m_wordmark);
    centerLayout->addSpacing(6);
    centerLayout->addWidget(m_tagline);
    centerLayout->addSpacing(36);
    centerLayout->addWidget(m_searchField, 0, Qt::AlignHCenter);
    centerLayout->addSpacing(10);
    centerLayout->addWidget(m_searchButton, 0, Qt::AlignHCenter);
    centerLayout->addSpacing(16);
    centerLayout->addWidget(m_hint, 0, Qt::AlignHCenter);

    rootLayout->addWidget(center, 0, Qt::AlignHCenter);
    rootLayout->addStretch(3);
    rootLayout->addWidget(m_version, 0, Qt::AlignHCenter);
    rootLayout->setContentsMargins(0, 0, 0, 26);

    connect(m_searchField, &QLineEdit::returnPressed, this, &NewTabPage::submit);
    connect(m_searchButton, &QPushButton::clicked, this, &NewTabPage::submit);

    restyle();
}

void NewTabPage::setSearchEngineName(const QString& name)
{
    m_engineName = name;
    m_searchButton->setText(name.isEmpty() ? tr("Search") : tr("Search with %1").arg(name));
}

void NewTabPage::setSearchEnginePlaceholder(const QString& text)
{
    if (!text.isEmpty()) {
        m_searchField->setPlaceholderText(text);
    }
}

void NewTabPage::setDarkMode(bool dark)
{
    m_dark = dark;
    restyle();
    update();
}

void NewTabPage::restyle()
{
    const auto c = m_dark ? Theme::darkColors() : Theme::lightColors();
    const QString family = Theme::fontFamily();
    const QColor accent(c.accent);

    // Letter spacing is set on the font, but the application stylesheet also
    // sets a font size, so the sizes below are declared in the stylesheet too.
    m_wordmark->setFont(spacedFont(family, 38, 8.0, QFont::Light));
    m_tagline->setFont(spacedFont(family, 8, 3.4, QFont::Normal));
    m_hint->setFont(spacedFont(family, 9, 0.6, QFont::Normal));
    m_version->setFont(spacedFont(family, 7, 1.6, QFont::Normal));

    m_version->setText(QStringLiteral("Yozora %1").arg(QString::fromLatin1(kVersionString)));

    const QString sheet = QStringLiteral(R"(
QWidget#newTabCenter { background: transparent; }
QLabel#newTabWordmark, QLabel#newTabTagline, QLabel#newTabHint, QLabel#newTabVersion {
    background: transparent;
    color: %MUTED%;
}
QLabel#newTabWordmark {
    color: %TEXT%;
    font-size: 38px;
    font-weight: 300;
}
QLabel#newTabTagline { font-size: 11px; }
QLabel#newTabHint { font-size: 12px; }
QLabel#newTabVersion { font-size: 10px; }
QLineEdit#newTabSearch {
    background: %FIELD%;
    color: %FIELD_TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 23px;
    padding: 0 22px;
    font-size: 14px;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
}
QLineEdit#newTabSearch:hover { border-color: %HOVER%; }
QLineEdit#newTabSearch:focus { border-color: %ACCENT%; }
QPushButton#newTabSearchButton {
    background: %ACCENT%;
    color: %ACCENT_TEXT%;
    border: none;
    border-radius: 22px;
    font-size: 13px;
    font-weight: 500;
}
QPushButton#newTabSearchButton:hover { background: %ACCENT_HOVER%; }
QPushButton#newTabSearchButton:pressed { background: %ACCENT_DOWN%; }
)")
                         .replace(QStringLiteral("%TEXT%"), c.text)
                         .replace(QStringLiteral("%MUTED%"), c.textMuted)
                         .replace(QStringLiteral("%FIELD%"), c.field)
                         .replace(QStringLiteral("%FIELD_TEXT%"), c.fieldText)
                         .replace(QStringLiteral("%BORDER%"), c.border)
                         .replace(QStringLiteral("%HOVER%"), c.surfaceActive)
                        .replace(QStringLiteral("%ACCENT%"), c.accent)
                        .replace(QStringLiteral("%ACCENT_HOVER%"), accent.lighter(112).name())
                        .replace(QStringLiteral("%ACCENT_DOWN%"), accent.darker(112).name())
                         .replace(QStringLiteral("%ACCENT_TEXT%"), c.accentText);

    setStyleSheet(sheet);
}

void NewTabPage::focusSearch()
{
    m_searchField->setFocus(Qt::OtherFocusReason);
    m_searchField->selectAll();
}

void NewTabPage::reset()
{
    m_searchField->clear();
}

void NewTabPage::submit()
{
    const QString query = m_searchField->text().trimmed();
    if (query.isEmpty()) {
        m_searchField->setFocus();
        return;
    }
    emit searchRequested(query);
}

void NewTabPage::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        m_searchField->clear();
        return;
    }
    QWidget::keyPressEvent(event);
}

void NewTabPage::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    paintSky(painter, event->rect(), m_dark);
    QWidget::paintEvent(event);
}

}  // namespace yozora
