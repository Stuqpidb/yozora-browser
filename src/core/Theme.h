// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class QApplication;
class QPalette;

namespace yozora {

// Visual identity of Yozora: a deep night sky with one cool accent. The
// palette deliberately avoids saturated brand colours — the interface is meant
// to feel like looking out of a window at night, not like a product landing
// page. Everything visual is defined here so the chrome, the home page and the
// dialogs can never drift apart.
class Theme {
public:
    struct Colors {
        QString background;      // window / chrome background
        QString surface;         // toolbar, tab strip background
        QString surfaceHover;    // hovered control background
        QString surfaceActive;   // pressed / active tab background
        QString tabActive;       // active tab background
        QString tabInactive;     // inactive tab background
        QString border;          // hairline separators
        QString text;            // primary text
        QString textMuted;       // secondary text (inactive tab labels)
        QString accent;          // brand accent
        QString accent2;         // brand accent, second stop
        QString accentText;      // text on the accent colour
        QString danger;          // destructive actions (close button hover)
        QString field;           // input field background
        QString fieldText;
        QString shadow;
        QString rail;            // left navigation rail background
        QString card;            // translucent card background
        QString cardBorder;      // card border
        QString chip;            // small pill / chip background
    };

    [[nodiscard]] static Colors darkColors();
    [[nodiscard]] static Colors lightColors();

    // Applies palette + stylesheet to the running application.
    static void apply(bool dark);

    // The Qt style sheet used for the browser chrome.
    [[nodiscard]] static QString styleSheet(bool dark);

    // Shared resource style block reused by the HTML surfaces (error page) so
    // they look like part of the same product.
    [[nodiscard]] static QString htmlStyle(bool dark);

    [[nodiscard]] static QString fontFamily();

    // Which theme is currently active.
    //
    // Every custom-painted widget asks this instead of inspecting the palette.
    // Once a style sheet is installed the palette stops being a reliable
    // signal — a widget can be light while the window is dark — and that
    // mismatch is what makes half-painted interfaces.
    [[nodiscard]] static bool isDark();
    static void setDark(bool dark);

    // A vertical accent gradient as a CSS string, e.g. for qlineargradient().
    [[nodiscard]] static QString accentGradient(const Colors& c);
};

}  // namespace yozora
