// SPDX-License-Identifier: MIT
#include "ui/LibraryDialog.h"

#include "core/BookmarkStore.h"
#include "core/HistoryStore.h"
#include "core/Theme.h"
#include "ui/Icons.h"

#include <QAbstractButton>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

namespace yozora {

namespace {

// Roles for the data hanging off a row. The url is the identity of the entry;
// the rest is only ever read back for the row's own labels.
constexpr int kUrlRole = Qt::UserRole + 1;
constexpr int kTitleRole = Qt::UserRole + 2;
constexpr int kHostRole = Qt::UserRole + 3;
constexpr int kTimeRole = Qt::UserRole + 4;

// A delegate would be the "proper" way to draw a custom row, but a QListWidget
// item with a couple of labels does the same job here and keeps the widget count
// down. The row is a small QWidget with three labels and a badge.
QString hostLabel(const QString& url)
{
    QString host = QUrl(url).host();
    if (host.startsWith(QLatin1String("www."))) {
        host.remove(0, 4);
    }
    return host.isEmpty() ? url : host;
}

}  // namespace

// A round badge with the first letter of the site, in a colour derived from the
// address so the same site always looks the same. The same idea as the pinned
// site tiles, so the two surfaces agree.
class SiteBadge : public QWidget {
public:
    SiteBadge(const QString& letter, const QString& seed, QWidget* parent = nullptr)
        : QWidget(parent)
    {
        const int hue = static_cast<int>(qHash(seed) % 360);
        m_color = QColor::fromHsl(hue, 148, 128);
        QFont font(Theme::displayFamily());
        font.setPointSizeF(11.0);
        font.setWeight(QFont::Bold);
        m_font = font;
        m_letter = letter.isEmpty() ? QStringLiteral("?") : letter.left(1).toUpper();
        setFixedSize(28, 28);
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        painter.setBrush(m_color);
        painter.drawEllipse(rect());
        painter.setFont(m_font);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, m_letter);
    }

private:
    QColor m_color;
    QFont m_font;
    QString m_letter;
};

LibraryDialog::LibraryDialog(bool bookmarks, BookmarkStore* bookmarkStore,
                             HistoryStore* historyStore, QWidget* parent)
    : QDialog(parent)
    , m_bookmarks(bookmarks)
    , m_bookmarkStore(bookmarkStore)
    , m_historyStore(historyStore)
{
    setWindowTitle(bookmarks ? tr("Bookmarks") : tr("History"));
    setMinimumSize(620, 520);
    resize(680, 600);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // --- header -------------------------------------------------------------
    auto* header = new QWidget(this);
    header->setObjectName(QStringLiteral("libraryHeader"));
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(22, 18, 22, 14);
    headerLayout->setSpacing(10);

    auto* titleRow = new QHBoxLayout;
    titleRow->setSpacing(10);
    auto* heading = new QLabel(bookmarks ? tr("Bookmarks") : tr("History"), header);
    heading->setObjectName(QStringLiteral("dialogTitle"));
    titleRow->addWidget(heading);
    m_count = new QLabel(header);
    m_count->setObjectName(QStringLiteral("dialogSubtitle"));
    titleRow->addWidget(m_count);
    titleRow->addStretch(1);
    headerLayout->addLayout(titleRow);

    m_filter = new QLineEdit(header);
    m_filter->setObjectName(QStringLiteral("searchBox"));
    m_filter->setPlaceholderText(bookmarks ? tr("Filter bookmarks") : tr("Search history"));
    m_filter->setClearButtonEnabled(true);
    m_filter->addAction(icons::icon(icons::Shape::Magnifier, 16, QColor(Theme::colors().textMuted)),
                        QLineEdit::LeadingPosition);
    headerLayout->addWidget(m_filter);
    root->addWidget(header);

    // --- list ---------------------------------------------------------------
    m_list = new QListWidget(this);
    m_list->setObjectName(QStringLiteral("libraryList"));
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setUniformItemSizes(false);
    m_list->setMouseTracking(true);
    m_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    root->addWidget(m_list, 1);

    // --- footer -------------------------------------------------------------
    auto* footer = new QWidget(this);
    footer->setObjectName(QStringLiteral("libraryFooter"));
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(22, 12, 22, 16);
    footerLayout->setSpacing(10);

    m_removeButton = new QPushButton(tr("Remove"), footer);
    m_removeButton->setObjectName(QStringLiteral("quietButton"));
    m_removeButton->setCursor(Qt::PointingHandCursor);
    m_removeButton->setEnabled(false);
    connect(m_removeButton, &QPushButton::clicked, this, &LibraryDialog::removeCurrent);
    footerLayout->addWidget(m_removeButton);
    footerLayout->addStretch(1);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    footerLayout->addWidget(buttons);
    root->addWidget(footer);

    // --- wiring -------------------------------------------------------------
    // Typing filters as it goes, but not on every keystroke: a long history is
    // thousands of rows and rebuilding them is not free. 120ms is below the
    // threshold where the list feels like it lags behind the keyboard.
    m_filterTimer = new QTimer(this);
    m_filterTimer->setSingleShot(true);
    m_filterTimer->setInterval(120);
    connect(m_filterTimer, &QTimer::timeout, this,
            [this] { applyFilter(m_filter->text()); });
    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString&) {
        m_filterTimer->start();
    });
    connect(m_filter, &QLineEdit::returnPressed, this, &LibraryDialog::openCurrent);

    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem*) { openCurrent(); });
    connect(m_list, &QListWidget::itemSelectionChanged, this,
            &LibraryDialog::updateButtons);
    connect(m_list, &QListWidget::itemDoubleClicked, this,
            [this](QListWidgetItem*) { openCurrent(); });
}

void LibraryDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    reload();
    // The filter box is where the user's hands already are when this window
    // opens, so it gets the keyboard. The list is one Tab away.
    m_filter->setFocus(Qt::OtherFocusReason);
}

QString LibraryDialog::relativeTime(qint64 epochMs)
{
    // The stores keep milliseconds; reading them as seconds would put every
    // entry thousands of years in the future and silently print no time at all.
    if (epochMs <= 0) {
        return QString();
    }
    const QDateTime then = QDateTime::fromMSecsSinceEpoch(epochMs);
    const qint64 seconds = then.secsTo(QDateTime::currentDateTime());
    if (seconds < 0) {
        return QString();
    }
    if (seconds < 60) {
        return tr("just now");
    }
    if (seconds < 3600) {
        // Translators: %1 is a number of minutes, e.g. "5".
        return tr("%1 min ago").arg(seconds / 60);
    }
    if (seconds < 86400) {
        // Translators: %1 is a number of hours, e.g. "3".
        return tr("%1 h ago").arg(seconds / 3600);
    }
    if (seconds < 7 * 86400) {
        // Translators: %1 is a number of days, e.g. "2".
        return tr("%1 d ago").arg(seconds / 86400);
    }
    return then.toString(QStringLiteral("d MMM yyyy"));
}

void LibraryDialog::setFilterText(const QString& text)
{
    m_filter->setText(text);
}

QString LibraryDialog::filterText() const
{
    return m_filter->text();
}

int LibraryDialog::visibleRowCount() const
{
    return m_list->count();
}

void LibraryDialog::reload()
{
    m_entries.clear();

    if (m_bookmarks) {
        const QList<Bookmark> items = m_bookmarkStore ? m_bookmarkStore->all()
                                                       : QList<Bookmark>{};
        for (const Bookmark& bookmark : items) {
            m_entries.append({bookmark.url, bookmark.title, hostLabel(bookmark.url),
                              bookmark.addedAt});
        }
    } else {
        const QList<HistoryEntry> items =
            m_historyStore ? m_historyStore->recent(1000) : QList<HistoryEntry>{};
        for (const HistoryEntry& entry : items) {
            m_entries.append({entry.url, entry.title, hostLabel(entry.url), entry.visitedAt});
        }
    }

    applyFilter(m_filter->text());
}

void LibraryDialog::applyFilter(const QString& needle)
{
    m_list->clear();

    const QString wanted = needle.trimmed();
    for (const Entry& entry : m_entries) {
        if (!wanted.isEmpty()
            && !entry.title.contains(wanted, Qt::CaseInsensitive)
            && !entry.host.contains(wanted, Qt::CaseInsensitive)
            && !entry.url.contains(wanted, Qt::CaseInsensitive)) {
            continue;
        }

        auto* item = new QListWidgetItem;
        item->setData(kUrlRole, entry.url);
        item->setData(kTitleRole, entry.title);
        item->setData(kHostRole, entry.host);
        item->setData(kTimeRole, entry.visitedMs);

        // The row: badge, title over host, time on the right.
        auto* row = new QWidget(m_list);
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(12, 7, 14, 7);
        layout->setSpacing(12);

        const QString letter = entry.title.isEmpty() ? entry.host : entry.title;
        layout->addWidget(new SiteBadge(letter, entry.url, row));

        auto* texts = new QVBoxLayout;
        texts->setContentsMargins(0, 0, 0, 0);
        texts->setSpacing(1);

        auto* title = new QLabel(entry.title.isEmpty() ? entry.host : entry.title, row);
        title->setObjectName(QStringLiteral("rowTitle"));
        texts->addWidget(title);

        auto* host = new QLabel(entry.host, row);
        host->setObjectName(QStringLiteral("rowSubtle"));
        texts->addWidget(host);
        layout->addLayout(texts, 1);

        const QString stamp = relativeTime(entry.visitedMs);
        if (!stamp.isEmpty()) {
            auto* when = new QLabel(stamp, row);
            when->setObjectName(QStringLiteral("rowTime"));
            layout->addWidget(when, 0, Qt::AlignVCenter);
        }

        item->setSizeHint(row->sizeHint().expandedTo(QSize(0, 52)));
        m_list->addItem(item);
        m_list->setItemWidget(item, row);
    }

    // Highlight the first row so "Enter opens the top result" works without the
    // user having to click first.
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
    updateHeader();
    updateButtons();
}

void LibraryDialog::updateHeader()
{
    const int shown = m_list->count();
    const int total = static_cast<int>(m_entries.size());
    if (shown == total) {
        m_count->setText(tr("%1").arg(total));
    } else {
        m_count->setText(tr("%1 of %2").arg(shown).arg(total));
    }
}

void LibraryDialog::updateButtons()
{
    m_removeButton->setEnabled(currentItem() != nullptr);
}

QListWidgetItem* LibraryDialog::currentItem() const
{
    QListWidgetItem* item = m_list->currentItem();
    if (item && !item->data(kUrlRole).toString().isEmpty()) {
        return item;
    }
    return nullptr;
}

void LibraryDialog::openCurrent()
{
    QListWidgetItem* item = currentItem();
    if (!item) {
        return;
    }
    m_chosen = QUrl(item->data(kUrlRole).toString());
    accept();
}

void LibraryDialog::removeCurrent()
{
    QListWidgetItem* item = currentItem();
    if (!item) {
        return;
    }
    const QString url = item->data(kUrlRole).toString();

    if (m_bookmarks) {
        if (m_bookmarkStore) {
            m_bookmarkStore->remove(url);
        }
    } else if (m_historyStore) {
        // The history store only exposes "clear everything", so a single entry
        // is removed by rewriting the list. See HistoryStore.
        m_historyStore->remove(url);
    }
    reload();
}

}  // namespace yozora
