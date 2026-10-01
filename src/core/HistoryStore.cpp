// SPDX-License-Identifier: MIT
#include "core/HistoryStore.h"

#include "app/AppPaths.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

namespace yozora {

namespace {
constexpr int kMaxEntries = 2000;
constexpr qint64 kDedupWindowMs = 60 * 1000;
}  // namespace

HistoryStore::HistoryStore(QObject* parent)
    : QObject(parent)
{
    load();
}

void HistoryStore::record(const QString& url, const QString& title)
{
    if (url.isEmpty() || url.startsWith(QLatin1String("about:"))
        || url.startsWith(QLatin1String("yozora-error:"))) {
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    // Refreshing the same page within a minute must not spam the list.
    if (!m_items.isEmpty() && m_items.first().url == url && now - m_items.first().visitedAt < kDedupWindowMs) {
        m_items.first().visitedAt = now;
        if (!title.isEmpty()) {
            m_items.first().title = title;
        }
        scheduleSave();
        emit changed();
        return;
    }

    // A repeat visit moves the entry to the top instead of duplicating it.
    for (int i = 1; i < m_items.size(); ++i) {
        if (m_items.at(i).url == url) {
            HistoryEntry entry = m_items.takeAt(i);
            entry.title = title.isEmpty() ? entry.title : title;
            entry.visitedAt = now;
            m_items.prepend(entry);
            scheduleSave();
            emit changed();
            return;
        }
    }

    m_items.prepend({url, title.isEmpty() ? url : title, now});
    while (m_items.size() > kMaxEntries) {
        m_items.removeLast();
    }
    scheduleSave();
    emit changed();
}

QList<HistoryEntry> HistoryStore::recent(int limit) const
{
    if (limit <= 0 || limit >= m_items.size()) {
        return m_items;
    }
    return m_items.mid(0, limit);
}

void HistoryStore::remove(const QString& url)
{
    // Every visit to the same address is its own entry, so this drops them all.
    const qsizetype before = m_items.size();
    m_items.removeIf([&url](const HistoryEntry& entry) { return entry.url == url; });
    if (m_items.size() == before) {
        return;
    }
    scheduleSave();
    emit changed();
}

void HistoryStore::clear()
{
    if (m_items.isEmpty()) {
        return;
    }
    m_items.clear();
    scheduleSave();
    emit changed();
}

void HistoryStore::load()
{
    QFile file(AppPaths::historyPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray array = document.isArray() ? document.array()
                                                : document.object().value(QStringLiteral("items")).toArray();
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        HistoryEntry entry;
        entry.url = object.value(QStringLiteral("url")).toString();
        entry.title = object.value(QStringLiteral("title")).toString();
        entry.visitedAt = static_cast<qint64>(
            object.value(QStringLiteral("visitedAt")).toDouble());
        if (!entry.url.isEmpty()) {
            m_items.append(entry);
        }
    }
}

void HistoryStore::scheduleSave()
{
    QTimer::singleShot(500, this, [this] {
        AppPaths::ensureCreated();
        QJsonArray array;
        for (const HistoryEntry& entry : m_items) {
            QJsonObject object;
            object.insert(QStringLiteral("url"), entry.url);
            object.insert(QStringLiteral("title"), entry.title);
            object.insert(QStringLiteral("visitedAt"), static_cast<double>(entry.visitedAt));
            array.append(object);
        }
        QSaveFile file(AppPaths::historyPath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(array).toJson(QJsonDocument::Compact));
            file.commit();
        }
    });
}

}  // namespace yozora
