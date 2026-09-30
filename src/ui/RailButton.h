// SPDX-License-Identifier: MIT
#pragma once

#include <QAbstractButton>

namespace yozora {

// The shapes in the left rail. Drawing them ourselves keeps the rail
// independent of whichever font happens to be installed: the Unicode symbols
// this used to rely on (house, hourglass, gear) render at wildly different
// sizes in different fonts, and a style-sheet font size silently overrode the
// point size set in code, so the icons ended up tiny inside their buttons.
enum class RailIcon {
    Home,
    History,
    Bookmarks,
    Downloads,
    Private,
    Settings,
};

// One icon in the left rail: a glass pill that lights up on hover, a filled
// state for the current location, and a vector glyph drawn in a 24x24 box.
class RailButton : public QAbstractButton {
    Q_OBJECT

public:
    explicit RailButton(RailIcon icon, const QString& tooltip, QWidget* parent = nullptr);

    [[nodiscard]] RailIcon icon() const { return m_icon; }
    void setIcon(RailIcon icon);

    [[nodiscard]] QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    RailIcon m_icon;
};

}  // namespace yozora
