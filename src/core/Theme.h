// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class QApplication;
class QPalette;

namespace yozora {

// Visual identity of Yozora: a deep night sky with one cool accent. The
// palette deliberately avoids saturated brand colours - the interface is meant
// to feel like looking out of a window at night, not like a product landing
// page. Everything visual is defined here so the chrome, the home page and the
// dialogs can never drift apart.
//
// There is exactly one theme. A light variant was tried and removed: on a
// night-sky product it looked like a different application, and every painted
// widget had to carry a light branch that never matched the artwork behind it.
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

    [[nodiscard]] static Colors colors();

    // Applies palette + stylesheet to the running application, after
    // registering the bundled typefaces.
    static void apply();

    // The Qt style sheet used for the browser chrome.
    [[nodiscard]] static QString styleSheet();

    // Shared resource style block reused by the HTML surfaces (error page) so
    // they look like part of the same product.
    [[nodiscard]] static QString htmlStyle();

    // Text face, and the display face used for the wordmark and headings.
    [[nodiscard]] static QString fontFamily();
    [[nodiscard]] static QString displayFamily();

    // Registers the fonts in resources/fonts. Safe to call more than once.
    static void loadFonts();

    // A vertical accent gradient as a CSS string, e.g. for qlineargradient().
    [[nodiscard]] static QString accentGradient(const Colors& c);
};

}  // namespace yozora
