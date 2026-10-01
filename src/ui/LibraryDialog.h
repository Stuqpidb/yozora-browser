// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <QList>
#include <QUrl>

class QAbstractButton;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QTimer;

namespace yozora {

class BookmarkStore;
class HistoryStore;

// The history / bookmarks window.
//
// This used to be a QListWidget in a bare QDialog: one line per entry, title and
// address glued together with a newline, and no way to find anything in a few
// hundred of them. It is now a searchable list of rows, each with a coloured
// site badge, a title, a host and a relative time, plus a filter box and a count
// in the header.
//
// The window is still a real QDialog, so it keeps the platform's frame, snapping
// and keyboard behaviour; only its contents are Yozora's.
class LibraryDialog : public QDialog {
    Q_OBJECT

public:
    LibraryDialog(bool bookmarks, BookmarkStore* bookmarkStore, HistoryStore* historyStore,
                  QWidget* parent = nullptr);

    // The row the user activated, if any. Checked by the caller after exec().
    [[nodiscard]] QUrl chosenUrl() const { return m_chosen; }

    // The filter box's text. Exposed so the behaviour can be tested without
    // reaching into the dialog's widgets.
    void setFilterText(const QString& text);
    [[nodiscard]] QString filterText() const;

    // How many rows the filter currently leaves visible. Equal to the entry
    // count when nothing is filtered out.
    [[nodiscard]] int visibleRowCount() const;

    // "2 hours ago", or an empty string when the time is unknown. Takes
    // milliseconds since the epoch, which is what HistoryStore and
    // BookmarkStore store.
    [[nodiscard]] static QString relativeTime(qint64 epochMs);

protected:
    void showEvent(QShowEvent* event) override;

private:
    // One row of the list.
    struct Entry {
        QString url;
        QString title;
        QString host;
        qint64 visitedMs = 0;
    };

    void reload();
    void applyFilter(const QString& needle);
    void openCurrent();
    void removeCurrent();
    void updateHeader();
    void updateButtons();
    [[nodiscard]] QListWidgetItem* currentItem() const;

    bool m_bookmarks = false;
    BookmarkStore* m_bookmarkStore = nullptr;
    HistoryStore* m_historyStore = nullptr;
    QList<Entry> m_entries;
    QUrl m_chosen;
    QLineEdit* m_filter = nullptr;
    QListWidget* m_list = nullptr;
    QLabel* m_count = nullptr;
    QAbstractButton* m_removeButton = nullptr;
    QTimer* m_filterTimer = nullptr;
};

}  // namespace yozora
