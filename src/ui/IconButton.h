// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QAbstractButton>

class QPaintEvent;
class QSize;

namespace yozora {

// A flat icon button: no frame, no text, just one of the vector shapes from
// Icons.h. Used where a Unicode glyph used to be, because the application
// style sheet pins the font size and shrank those glyphs to nothing.
class IconButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit IconButton(icons::Shape shape, QWidget* parent = nullptr);

    void setShape(icons::Shape shape);
    void setIconSize(int size);
    void setMuted(bool muted);

protected:
    void paintEvent(QPaintEvent* event) override;
    [[nodiscard]] QSize sizeHint() const override;

private:
    [[nodiscard]] QColor inkColor() const;

    icons::Shape m_shape = icons::Shape::More;
    int m_size = 18;
    bool m_muted = false;
};

}  // namespace yozora
