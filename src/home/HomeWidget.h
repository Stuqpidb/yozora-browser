// SPDX-License-Identifier: MIT
#pragma once

#include <QFrame>
#include <QJsonObject>
#include <QPaintEvent>
#include <QPoint>
#include <QRect>
#include <QString>

#include <functional>

class QLabel;
class QMouseEvent;
class QToolButton;
class QVBoxLayout;
class QHBoxLayout;

namespace yozora {

class BookmarkStore;
class HistoryStore;

// Everything a home widget may need from the browser. Kept as plain pointers
// and callbacks so widgets never reach into the window themselves.
struct HomeContext {
    BookmarkStore* bookmarks = nullptr;
    HistoryStore* history = nullptr;
    std::function<int()> openTabCount;
    std::function<quint64()> blockedTrackerCount;
    std::function<void()> openHistory;
    std::function<void()> openBookmarks;
    std::function<void()> openSettings;
};

// Base class for every home-page widget (a "card").
//
// A card has a header with a drag handle, per-card menu and a body. Widgets
// are freely placed and resized by dragging: the header drags the card, the
// bottom-right corner resizes it, and the home page stores the result.
//
// Cards marked fixed (the search hero) are pinned to the top of the page and
// cannot be moved, resized or removed, exactly like the Chrome start page.
class HomeWidget : public QFrame {
    Q_OBJECT

public:
    explicit HomeWidget(const HomeContext& context, QWidget* parent = nullptr);

    [[nodiscard]] virtual QString typeId() const = 0;
    [[nodiscard]] virtual QString displayName() const = 0;

    [[nodiscard]] QSize cardSize() const { return m_cardSize; }
    void setCardSize(const QSize& size);

    [[nodiscard]] bool isFixed() const { return m_fixed; }
    void setFixed(bool fixed);
    void setHeaderVisible(bool visible);

    // type + footprint + subclass settings. The position on the page is owned
    // by the HomePage, not by the card.
    [[nodiscard]] QJsonObject save() const;
    void restore(const QJsonObject& object);

    // Adds a small control at the right of the card header (e.g. "View all").
    void addHeaderAction(QWidget* widget);

signals:
    void dragBegin();
    void dragMove(const QPoint& globalPos);
    void dragDrop(const QPoint& globalPos);
    void removeRequested();
    void changed();

protected:
    // Where subclasses place their content.
    [[nodiscard]] QWidget* body() const { return m_body; }
    [[nodiscard]] QVBoxLayout* bodyLayout() const;
    void setTitle(const QString& title);
    [[nodiscard]] const HomeContext& context() const { return m_context; }

    // Serialisation hooks for subclasses.
    [[nodiscard]] virtual QJsonObject saveSettings() const { return {}; }
    virtual void loadSettings(const QJsonObject&) {}

    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    // The card body is painted here rather than by a style sheet, because glass
    // needs a shadow, a top highlight and a grain that QSS cannot express.
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] QRect headerRect() const;
    [[nodiscard]] bool isOverResizeCorner(const QPoint& pos) const;
    void showCardMenu(const QPoint& globalPos);

    HomeContext m_context;
    QLabel* m_title = nullptr;
    QToolButton* m_menuButton = nullptr;
    QHBoxLayout* m_header = nullptr;
    QLabel* m_handle = nullptr;
    QWidget* m_body = nullptr;
    QSize m_cardSize = QSize(360, 260);

    // Current gesture.
    QPoint m_pressOffset;
    QSize m_startSize;
    bool m_fixed = false;
    bool m_dragging = false;   // header drag in progress
    bool m_resizing = false;   // corner resize in progress
};

}  // namespace yozora
