// SPDX-License-Identifier: MIT
#include "core/BookmarkStore.h"

#include "app/AppPaths.h"

#include <QDateTime>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTimer>

namespace yozora {

BookmarkStore::BookmarkStore(QObject* parent)
    : QObject(parent)
{
    load();
}

bool BookmarkStore::contains(const QString& url) const
{
    for (const Bookmark& bookmark : m_items) {
        if (bookmark.url == url) {
            return true;
        }
    }
    return false;
}

void BookmarkStore::add(const QString& url, const QString& title)
{
    if (url.isEmpty() || contains(url)) {
        return;
    }
    m_items.append({url, title.isEmpty() ? url : title, QDateTime::currentMSecsSinceEpoch()});
    scheduleSave();
    emit changed();
}

void BookmarkStore::remove(const QString& url)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).url == url) {
            m_items.removeAt(i);
            scheduleSave();
            emit changed();
            return;
        }
    }
}

void BookmarkStore::toggle(const QString& url, const QString& title)
{
    if (contains(url)) {
        remove(url);
    } else {
        add(url, title);
    }
}

void BookmarkStore::load()
{
    QFile file(AppPaths::bookmarksPath());
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    const QJsonArray array = document.isArray() ? document.array()
                                                : document.object().value(QStringLiteral("items")).toArray();
    for (const QJsonValue& value : array) {
        const QJsonObject object = value.toObject();
        Bookmark bookmark;
        bookmark.url = object.value(QStringLiteral("url")).toString();
        bookmark.title = object.value(QStringLiteral("title")).toString();
        bookmark.addedAt = static_cast<qint64>(
            object.value(QStringLiteral("addedAt")).toDouble());
        if (!bookmark.url.isEmpty()) {
            m_items.append(bookmark);
        }
    }
}

void BookmarkStore::scheduleSave()
{
    // Coalesce bursts of edits into one write.
    QTimer::singleShot(300, this, [this] {
        AppPaths::ensureCreated();
        QJsonArray array;
        for (const Bookmark& bookmark : m_items) {
            QJsonObject object;
            object.insert(QStringLiteral("url"), bookmark.url);
            object.insert(QStringLiteral("title"), bookmark.title);
            object.insert(QStringLiteral("addedAt"), static_cast<double>(bookmark.addedAt));
            array.append(object);
        }
        QSaveFile file(AppPaths::bookmarksPath());
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(array).toJson(QJsonDocument::Indented));
            file.commit();
        }
    });
}

}  // namespace yozora
