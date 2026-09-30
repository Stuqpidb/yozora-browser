// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QFrame>
#include <QString>

class QLineEdit;
class QMouseEvent;
class QPainter;

namespace yozora {

// A glass text field: the surface, the icons and the focus ring are all drawn
// by hand so the field looks the same in the toolbar and on the start page.
//
// The text itself lives in a real child QLineEdit instead of this class being
// one. A widget may only have one QPainter on it at a time, and QLineEdit
// paints its own frame and text, so subclassing it would mean painting over a
// live painter. Owning the editor as a child keeps the two paints separate and
// leaves caret movement, selection, clipboard and IME to the platform.
class GlassField : public QFrame {
    Q_OBJECT

public:
    enum class Trailing {
        None,
        Arrow,   // a "go" arrow, shown when there is something to open
        Clear,   // a small cross, shown while there is text
    };

    explicit GlassField(QWidget* parent = nullptr);

    QLineEdit* editor() const { return m_editor; }

    // The QLineEdit surface callers actually use, forwarded to the child.
    QString text() const;
    void setText(const QString& text);
    void clear();
    void setPlaceholderText(const QString& text);
    void setFocus();
    void setFocus(Qt::FocusReason reason);
    void selectAll();
    bool hasFocus() const;

    void setTrailing(Trailing trailing);
    void setRadius(qreal radius);
    // A larger field for the start page: bigger icons, more padding.
    void setHeroMode(bool hero);
    // Reserves empty space around the surface for its outer shadow. The panel is
    // inset by this much, so the caller has to size the widget accordingly.
    void setShadowMargin(qreal margin);

    void setFocusPolicy(Qt::FocusPolicy policy);
    void setCursorPosition(int position);

signals:
    void returnPressed();
    void textChanged(const QString& text);
    void editingFinished();
    void trailingClicked();
    // The child editor took focus; the container itself never does.
    void editorFocused();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void layoutEditor();
    [[nodiscard]] qreal iconSize() const;
    [[nodiscard]] QRectF trailingBox() const;
    // The pill itself: the widget rect minus the shadow margin.
    [[nodiscard]] QRectF surfaceRect() const;
    void drawIcon(QPainter& painter, icons::Shape shape, const QRectF& box,
                  const QColor& color) const;

    QLineEdit* m_editor = nullptr;
    Trailing m_trailing = Trailing::Arrow;
    qreal m_radius = 19.0;
    qreal m_margin = 0.0;
    bool m_hero = false;
};

}  // namespace yozora
