// SPDX-License-Identifier: MIT
#pragma once

#include <QColor>
#include <QPainterPath>
#include <QRectF>
#include <QString>

class QPainter;
class QWidget;

namespace yozora {

// The "liquid glass" layer.
//
// What is real and what is not, stated plainly so nobody is misled later:
//
//   * The window backdrop is real. On Windows 11 the top-level window gets the
//     native DWM Mica (or Acrylic) material, so the title bar and any
//     non-painted area blur whatever is behind the window. On Windows 10 the
//     same call falls back to the older AccentPolicy blur. Everywhere else the
//     call is a no-op and the window simply stays opaque.
//
//   * The surfaces inside the window are not a real material. Qt Widgets cannot
//     sample the pixels behind an arbitrary child widget, so a frosted look is
//     built the way it is done in every desktop Qt app: a translucent fill, a
//     bright hairline along the top edge where light would catch it, a soft
//     outer shadow, and a little grain to break up the flatness.
//
// Everything here is a pure function of the theme, so the chrome, the start
// page and the dialogs cannot drift apart.
class Glass {
public:
    struct Recipe {
        QColor fill;       // translucent body of the surface
        QColor fillTop;    // slightly lighter at the top (light from above)
        QColor stroke;     // hairline border
        QColor highlight;  // the bright 1px specular line along the top edge
        QColor shadow;     // soft drop shadow
        qreal radius = 18;
        qreal borderWidth = 1.0;
        bool grain = true;
    };

    // Builds the glass recipe for the current theme.
    [[nodiscard]] static Recipe recipe(bool dark, qreal radius = 18);

    // Fills a rounded rectangle with the translucent body, the top-lit
    // gradient and the specular hairline. Does not draw the shadow.
    static void paintPanel(QPainter& painter, const QRectF& rect, const Recipe& recipe,
                           qreal opacity = 1.0);

    // A softer, lighter variant for nested surfaces (a chip inside a card).
    static void paintChip(QPainter& painter, const QRectF& rect, const Recipe& recipe,
                          qreal opacity = 1.0);

    // The specular hairline alone, for surfaces that are painted by the widget
    // itself (the address bar, a tab).
    static void paintTopHighlight(QPainter& painter, const QRectF& rect, const Recipe& recipe,
                                  qreal opacity = 1.0);

    // Draws the surface shadow *before* the panel, so it is never painted over.
    static void paintShadow(QPainter& painter, const QRectF& rect, const Recipe& recipe,
                            qreal opacity = 1.0);

    // The iOS-style "squircle": a superellipse. Used for the site tiles so they
    // read as icons rather than as rounded squares.
    [[nodiscard]] static QPainterPath squircle(const QRectF& rect, qreal radius);

    // A very light film of noise. Static pattern, so it costs one small pixmap
    // and never flickers.
    static void paintGrain(QPainter& painter, const QRectF& rect, const QColor& tint,
                           qreal opacity = 0.05);

    // Requests the native window backdrop. Safe to call on any platform and any
    // window state; it does nothing where the effect is unavailable.
    static void applyWindowBackdrop(QWidget* window, bool dark);

    // True when the platform actually gave us a blurred backdrop.
    [[nodiscard]] static bool windowBackdropActive();
};

}  // namespace yozora
