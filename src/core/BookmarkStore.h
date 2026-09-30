// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QObject>
#include <QString>

namespace yozora {

struct Bookmark {
    QString url;
    QString title;
    qint64 addedAt = 0;
};

// Local, private bookmark store. Plain JSON under AppPaths::stateDir(); there
// is no sync and no account.
class BookmarkStore : public QObject {
    Q_OBJECT

public:
    explicit BookmarkStore(QObject* parent = nullptr);

    [[nodiscard]] QList<Bookmark> all() const { return m_items; }
    [[nodiscard]] bool contains(const QString& url) const;
    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }

    void add(const QString& url, const QString& title);
    void remove(const QString& url);
    void toggle(const QString& url, const QString& title);

signals:
    void changed();

private:
    void load();
    void scheduleSave();

    QList<Bookmark> m_items;
};

}  // namespace yozora
