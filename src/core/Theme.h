// SPDX-License-Identifier: MIT
#pragma once

#include <QString>

class QApplication;
class QPalette;

namespace yozora {

// Visual identity of Yozora: a calm, dark "night sky" interface with a single
// cool accent. Kept in one place so the shell, the new tab page and the
// settings dialog can never drift apart.
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
        QString accentText;      // text on the accent colour
        QString danger;          // destructive actions (close button hover)
        QString field;           // input field background
        QString fieldText;
        QString shadow;
    };

    [[nodiscard]] static Colors darkColors();
    [[nodiscard]] static Colors lightColors();

    // Applies palette + stylesheet to the running application.
    static void apply(bool dark);

    // The Qt style sheet used for the browser chrome.
    [[nodiscard]] static QString styleSheet(bool dark);

    // Shared resource style block reused by the HTML surfaces (new tab page,
    // settings page, error page) so they look like part of the same product.
    [[nodiscard]] static QString htmlStyle(bool dark);

    [[nodiscard]] static QString fontFamily();
};

}  // namespace yozora
