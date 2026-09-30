// SPDX-License-Identifier: MIT
#pragma once

#include <QSize>

class QPainter;

namespace yozora {

// The Yozora night sky, painted rather than loaded from a bitmap.
//
// Everything is generated from a fixed seed in normalised coordinates, so the
// same window size always produces the same sky and resizing the window does not
// reshuffle the stars. The result is expensive enough (a few thousand tiny
// shapes) that it is rendered once into a cached pixmap and blitted afterwards.
class NightSky {
public:
    // Paints the sky over the whole of `logicalSize`, in device pixels.
    static void paint(QPainter& painter, const QSize& logicalSize, qreal devicePixelRatio);
};

}  // namespace yozora
