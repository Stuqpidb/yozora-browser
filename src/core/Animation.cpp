// SPDX-License-Identifier: MIT
#include "core/Animation.h"

#include <QEasingCurve>
#include <QVariantAnimation>

namespace yozora {

void Animation::configure(QVariantAnimation* animation, int durationMs, qreal from, qreal to)
{
    if (!animation) {
        return;
    }
    // Out, not in-out: a control that is being pressed should react at once and
    // then settle. A symmetric curve reads as sluggish at the start, which is
    // exactly where the user is watching.
    animation->setDuration(durationMs);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(QEasingCurve::OutCubic);
}

void Animation::start(QVariantAnimation* animation, qreal from, qreal to)
{
    if (!animation) {
        return;
    }
    animation->stop();
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->start();
}

}  // namespace yozora
