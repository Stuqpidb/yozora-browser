// SPDX-License-Identifier: MIT
#pragma once

#include "home/HomeWidget.h"

#include <QHash>
#include <QList>
#include <QPoint>
#include <QUrl>
#include <QWidget>

class QScrollArea;
class QToolButton;
class QTimer;

namespace yozora {

class SearchWidget;

// The Yozora home page: a night-sky canvas.
//
// The search hero is pinned to the top of the page (like the Chrome start
// page); every other widget is freely placeable — the header drags a card to
// any position, the card's bottom-right corner resizes it, and the arrangement
// is saved locally as fractional x/width plus pixel y/height.
class HomePage : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(const HomeContext& context, QWidget* parent = nullptr);
    ~HomePage() override;

    // Puts the caret in the pinned search field.
    void focusSearch();

signals:
    void openUrl(const QUrl& url);
    void searchRequested(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct Placement {
        qreal x = 0;  // 0..1 of the canvas width
        int y = 0;    // px from the canvas top
        qreal w = 0;  // 0..1 of the canvas width
        int h = 0;    // px
    };

    HomeWidget* createWidget(const QString& type);
    void buildDefaultLayout();
    void loadLayout();
    void placeWidget(HomeWidget* widget, const Placement& placement, bool userPlaced);
    void layoutCanvas();
    void addWidgetOfType(const QString& type);
    void addDefaultWidget(const QString& type, Placement placement);
    void removeWidget(HomeWidget* widget);
    void onWidgetChanged();
    void scheduleSave();
    void saveLayout() const;
    void showAddMenu();
    void beginDrag(HomeWidget* widget);
    void updateDrag(const QPoint& globalPos);
    void finishDrag();
    void relayoutHero();

    HomeContext m_context;
    QScrollArea* m_scroll = nullptr;
    QWidget* m_canvas = nullptr;
    QToolButton* m_addButton = nullptr;
    SearchWidget* m_hero = nullptr;
    QList<HomeWidget*> m_widgets;
    QHash<HomeWidget*, Placement> m_placements;
    QTimer* m_saveTimer = nullptr;

    HomeWidget* m_dragWidget = nullptr;
    QPoint m_dragOffset;
};

}  // namespace yozora
