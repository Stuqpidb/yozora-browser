// SPDX-License-Identifier: MIT
#pragma once

#include <QObject>

class QVariantAnimation;

namespace yozora {

// The one place Yozora decides how fast things move.
//
// Every transition in the interface runs through here, because the alternative
// is a duration and an easing curve per call site, and those drift apart within
// a release. Two things are deliberately different: `Quick` for feedback that
// has to feel instant (a button lighting up under the pointer) and `Standard`
// for anything that changes the layout (the rail collapsing, a dialog opening).
class Animation {
public:
    // Hover and press feedback. Short enough that it registers as immediate.
    static constexpr int kQuickMs = 110;
    // Layout changes and surfaces appearing.
    static constexpr int kStandardMs = 190;
    // Slow, for the largest surfaces.
    static constexpr int kSlowMs = 260;

    // Fills `animation` with the standard ease-out curve, so callers do not
    // each reinvent it. The animation is not started.
    static void configure(QVariantAnimation* animation, int durationMs, qreal from = 0.0,
                          qreal to = 1.0);

    // Starts `animation`, restarting it from `from` if it is already running.
    // Restarting rather than jumping is what makes a rapid second hover look
    // like the highlight retracting instead of snapping back.
    static void start(QVariantAnimation* animation, qreal from, qreal to);
};

}  // namespace yozora
