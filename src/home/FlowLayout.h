// SPDX-License-Identifier: MIT
#pragma once

#include <QLayout>
#include <QList>
#include <QRect>
#include <QStyle>

// A layout that lays widgets out left to right and wraps to a new line when
// the row is full.
//
// The home page puts a variable number of chips and icons into a fixed-width
// card, which a QHBoxLayout cannot do: it squeezes the widgets until their
// labels are unreadable instead of wrapping. Qt's own FlowLayout example is
// the same idea, written down here so the home page owns it.
class FlowLayout : public QLayout {
    Q_OBJECT

public:
    explicit FlowLayout(QWidget* parent, int margin = -1, int horizontalSpacing = -1,
                        int verticalSpacing = -1);
    ~FlowLayout() override;

    void addItem(QLayoutItem* item) override;
    [[nodiscard]] int horizontalSpacing() const;
    [[nodiscard]] int verticalSpacing() const;
    void setHorizontalSpacing(int spacing);
    void setVerticalSpacing(int spacing);

    [[nodiscard]] QSize sizeHint() const override;
    [[nodiscard]] QSize minimumSize() const override;
    [[nodiscard]] int count() const override;
    QLayoutItem* itemAt(int index) const override;
    QLayoutItem* takeAt(int index) override;
    [[nodiscard]] Qt::Orientations expandingDirections() const override;
    [[nodiscard]] bool hasHeightForWidth() const override;
    [[nodiscard]] int heightForWidth(int width) const override;
    void setGeometry(const QRect& rect) override;

private:
    // Lays out one row of items inside `rect` and returns the height used.
    int doLayout(const QRect& rect, bool apply) const;
    int smartSpacing(QStyle::PixelMetric metric) const;

    QList<QLayoutItem*> m_items;
    int m_horizontalSpacing;
    int m_verticalSpacing;
};
