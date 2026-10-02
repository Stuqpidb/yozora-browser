// SPDX-License-Identifier: MIT
#pragma once

#include <QIcon>
#include <QList>
#include <QRect>
#include <QString>
#include <QWidget>

namespace yozora {

// The Yozora tab strip: a fully custom strip with pill-shaped tabs and the
// product wordmark on the left, so it never looks like a stock browser tab bar.
// Tabs can be selected, closed, dragged to reorder, and created from the "+".
class TabStrip : public QWidget {
    Q_OBJECT

public:
    struct Tab {
        QString title;
        QIcon icon;
        QString tooltip;
        bool pinned = false;
    };

    explicit TabStrip(QWidget* parent = nullptr);

    void setTabs(const QList<Tab>& tabs);
    void setCurrentIndex(int index);
    [[nodiscard]] int currentIndex() const { return m_current; }
    [[nodiscard]] int count() const { return static_cast<int>(m_tabs.size()); }

    // Marks a tab as pinned. Pinned tabs are drawn narrow (icon only) and stay
    // at the left end; the window owns the ordering, the strip only reflects it.
    void setPinned(int index, bool pinned);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSizeHint() const override;

signals:
    void currentChanged(int index);
    void closeRequested(int index);
    void newTabRequested();
    void moveRequested(int from, int to);
    void contextMenuRequested(int index, const QPoint& globalPos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    // The hover highlight of every tab, as an animated 0..1 value each. Keeping
    // one value per tab (rather than a single index) is what makes the
    // highlight glide from one tab to the next instead of jumping.
    class HoverTrack {
    public:
        void resize(int count);
        void set(int index, qreal target);
        [[nodiscard]] qreal value(int index) const;
        void advance(qreal step);
        [[nodiscard]] bool atRest() const;

    private:
        QList<qreal> m_values;
    };

    [[nodiscard]] QRect tabRect(int index) const;
    [[nodiscard]] QRect closeRect(int index) const;
    [[nodiscard]] QRect plusRect() const;
    [[nodiscard]] int tabAt(const QPoint& pos) const;
    [[nodiscard]] int dropIndexFor(const QPoint& pos) const;
    [[nodiscard]] int tabWidth() const;
    [[nodiscard]] int pinnedTabWidth() const;
    void startHoverAnimation();

    QList<Tab> m_tabs;
    int m_current = -1;
    int m_hover = -1;
    HoverTrack m_hoverAmount;
    // True while the highlight is still moving, so the strip repaints itself.
    bool m_hoverAnimating = false;
    bool m_closeHover = false;
    bool m_plusHover = false;

    bool m_pressed = false;
    int m_pressIndex = -1;
    QPoint m_pressPos;
    bool m_dragging = false;
    int m_dragIndex = -1;
    QPoint m_dragPos;
};

}  // namespace yozora
