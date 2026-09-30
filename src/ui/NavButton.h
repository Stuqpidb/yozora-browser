// SPDX-License-Identifier: MIT
#pragma once

#include "ui/Icons.h"

#include <QAbstractButton>

namespace yozora {

// A toolbar button: a glass pill on hover and while pressed, a filled state
// for toggle buttons, and a drawn icon.
//
// Like the rail icons, these replace Unicode glyphs, which the application
// style sheet shrank to 13px regardless of the point size set in code.
class NavButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit NavButton(icons::Shape shape, const QString& tooltip, QWidget* parent = nullptr);

    void setShape(icons::Shape shape);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    icons::Shape m_shape;
};

}  // namespace yozora
