// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QLabel>

class QPaintEvent;

namespace yozora {

// A label that shows one of the vector shapes from Icons.h instead of a
// character, so it keeps its size no matter what the style sheet says.
class IconLabel : public QLabel {
    Q_OBJECT

public:
    explicit IconLabel(icons::Shape shape, QWidget* parent = nullptr);

    void setShape(icons::Shape shape);
    void setIconSize(int size);
    void setMuted(bool muted);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    [[nodiscard]] QColor inkColor() const;

    icons::Shape m_shape = icons::Shape::Drag;
    int m_size = 16;
    bool m_muted = true;
};

}  // namespace yozora
