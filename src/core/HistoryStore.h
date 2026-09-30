// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QObject>
#include <QString>

namespace yozora {

struct HistoryEntry {
    QString url;
    QString title;
    qint64 visitedAt = 0;
};

// Local browsing history. On by default, lives only on this machine under
// AppPaths::stateDir(), and can be cleared from the privacy settings.
class HistoryStore : public QObject {
    Q_OBJECT

public:
    explicit HistoryStore(QObject* parent = nullptr);

    void record(const QString& url, const QString& title);
    [[nodiscard]] QList<HistoryEntry> recent(int limit) const;
    [[nodiscard]] int count() const { return static_cast<int>(m_items.size()); }
    void clear();

signals:
    void changed();

private:
    void load();
    void scheduleSave();

    QList<HistoryEntry> m_items;  // newest first
};

}  // namespace yozora
