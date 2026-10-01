// SPDX-License-Identifier: MIT
#include "core/NightSky.h"

#include <QColor>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRandomGenerator>
#include <QtMath>

#include <cmath>

namespace yozora {

namespace {

// --- palette -----------------------------------------------------------------
const QColor kDeepSpace(0x05, 0x07, 0x12);
const QColor kUpperSpace(0x0d, 0x11, 0x28);
const QColor kLowerSpace(0x08, 0x0a, 0x1a);
const QColor kNebulaViolet(0x7a, 0x4b, 0xff);
const QColor kNebulaBlue(0x2f, 0x6d, 0xff);
const QColor kNebulaTeal(0x18, 0x8f, 0xa8);
const QColor kNebulaRose(0xc0, 0x53, 0x8f);
const QColor kStarWarm(0xff, 0xf0, 0xd4);
const QColor kStarCool(0xd8, 0xe6, 0xff);
const QColor kMilkyGlow(0xc8, 0xd4, 0xff);

// A soft blob of colour. Radial gradients are the only cheap way to get a
// nebula that has no visible edge.
void glow(QPainter& painter, const QPointF& center, qreal radius, const QColor& color, qreal peak)
{
    QRadialGradient gradient(center, radius);
    gradient.setColorAt(0.0, QColor(color.red(), color.green(), color.blue(),
                                   static_cast<int>(255 * peak)));
    gradient.setColorAt(0.45, QColor(color.red(), color.green(), color.blue(),
                                    static_cast<int>(255 * peak * 0.35)));
    gradient.setColorAt(1.0, QColor(color.red(), color.green(), color.blue(), 0));
    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawEllipse(center, radius, radius);
}

// One star: a bright core, a faint halo, and - for the brightest handful - a
// four-point flare. The halo is what makes a dot read as a star rather than as
// dust on the screen.
void star(QPainter& painter, const QPointF& at, qreal radius, qreal brightness,
          const QColor& tint, bool flare)
{
    glow(painter, at, radius * 7.0, tint, 0.20 * brightness);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tint.red(), tint.green(), tint.blue(),
                            static_cast<int>(255 * qBound(0.0, brightness, 1.0))));
    painter.drawEllipse(at, radius, radius);

    if (!flare) {
        return;
    }
    // Diffraction spikes, drawn as thin translucent triangles rather than lines
    // so they fade out towards the tips.
    const qreal length = radius * 11.0;
    const qreal width = radius * 0.9;
    QColor spike(tint.red(), tint.green(), tint.blue(), 110);
    for (int i = 0; i < 4; ++i) {
        const qreal angle = i * M_PI / 2.0;
        QPainterPath ray;
        const QPointF dir(std::cos(angle), std::sin(angle));
        const QPointF side(-dir.y(), dir.x());
        ray.moveTo(at + side * width);
        ray.lineTo(at + dir * length);
        ray.lineTo(at - side * width);
        ray.closeSubpath();
        QLinearGradient fade(at, at + dir * length);
        fade.setColorAt(0.0, spike);
        fade.setColorAt(1.0, QColor(tint.red(), tint.green(), tint.blue(), 0));
        painter.setBrush(fade);
        painter.setPen(Qt::NoPen);
        painter.drawPath(ray);
    }
}

// The Milky Way: a soft diagonal band of unresolved stars with a dark dust lane
// through it. Without the lane it reads as a smudge; with it, as depth.
//
// It is built from a chain of overlapping radial glows rather than one
// gradient-filled rectangle. A rectangle has to be bounded, and both of its
// straight edges stay visible no matter how the gradient runs - which is what
// made an earlier version look like flat diagonal stripes across the page.
void milkyWay(QPainter& painter, const QSize& size, QRandomGenerator& random)
{
    const qreal w = size.width();
    const qreal h = size.height();
    const qreal diagonal = std::hypot(w, h);
    constexpr qreal kAngle = -0.42;  // radians, from lower left to upper right

    painter.save();
    painter.setPen(Qt::NoPen);
    painter.translate(w * 0.5, h * 0.46);
    painter.rotate(kAngle * 180.0 / M_PI);

    const qreal bandHalf = h * 0.42;

    // The glow of the band itself, brightest in the middle and fading out at
    // both ends of the sky.
    const int puffs = 16;
    for (int i = 0; i < puffs; ++i) {
        const qreal t = static_cast<qreal>(i) / (puffs - 1);
        const qreal x = (t - 0.5) * diagonal * 1.15;
        // A slight vertical drift keeps the band from looking like a ruler.
        const qreal y = std::sin(t * 5.1) * h * 0.06;
        // Falls off towards the edges of the page.
        const qreal along = std::sin(t * M_PI);
        glow(painter, QPointF(x, y), bandHalf, kMilkyGlow, 0.055 * along);
    }

    // The dust lane: a chain of dark glows just off the centre line. Darkening
    // is painting translucent black, so it follows the same falloff as the light
    // - but it has to stay faint. A dozen overlapping discs at high opacity
    // stack up into a row of hard black blobs, which is worse than no lane.
    const int lanes = 22;
    for (int i = 0; i < lanes; ++i) {
        const qreal t = static_cast<qreal>(i) / (lanes - 1);
        const qreal x = (t - 0.5) * diagonal * 1.05;
        const qreal y = h * 0.012 + std::sin(t * 4.3) * h * 0.03;
        const qreal along = std::sin(t * M_PI);
        glow(painter, QPointF(x, y), h * 0.16, QColor(0x02, 0x03, 0x0a), 0.10 * along);
    }

    // Unresolved stars: dense, tiny, clustered towards the middle of the band.
    const int grains = static_cast<int>(qBound(900.0, size.width() * 2.2, 7000.0));
    for (int i = 0; i < grains; ++i) {
        const qreal bias = (random.bounded(1000) / 1000.0 - 0.5);
        const qreal x = bias * diagonal;
        // Gaussian-ish distribution across the band, so the edges thin out.
        const qreal across = (random.bounded(1000) / 1000.0 - 0.5) * 2.0;
        const qreal y = across * bandHalf * (0.55 + 0.45 * std::cos(bias * 2.2));
        if (qAbs(bias) > 0.42 && random.bounded(100) < 65) {
            continue;
        }
        const int alpha = 24 + random.bounded(80);
        const QColor tone = random.bounded(100) < 30 ? kStarWarm : kStarCool;
        painter.setBrush(QColor(tone.red(), tone.green(), tone.blue(), alpha));
        painter.drawRect(QRectF(x, y, 1, 1));
    }
    painter.restore();
}

void render(QPixmap* target, const QSize& size, qreal dpr)
{
    // QPixmap has no resize(): a default-constructed one is null, and a QPainter
    // on a null pixmap is inactive, so every draw call below would be silently
    // dropped. It has to be assigned a real one.
    *target = QPixmap(size);
    target->setDevicePixelRatio(dpr);
    target->fill(Qt::transparent);

    QPainter painter(target);
    if (!painter.isActive()) {
        qWarning("NightSky: could not paint the sky (%dx%d)", size.width(), size.height());
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal w = size.width();
    const qreal h = size.height();

    // Base sky: darkest at the top, a little lighter and warmer towards the
    // horizon so the page has a direction.
    QLinearGradient base(0, 0, 0, h);
    base.setColorAt(0.00, kUpperSpace);
    base.setColorAt(0.55, kDeepSpace);
    base.setColorAt(1.00, kLowerSpace);
    painter.fillRect(QRectF(0, 0, w, h), base);

    // Nebula field. Several overlapping blobs of different hue read as depth;
    // one big blob reads as a gradient.
    QRandomGenerator nebula(0x2F1B);
    const struct {
        qreal x;
        qreal y;
        qreal spread;
        int peak;
        QColor color;
    } clouds[] = {
        {0.14, 0.10, 0.62, 26, kNebulaViolet},
        {0.86, 0.34, 0.52, 20, kNebulaBlue},
        {0.62, 0.92, 0.58, 16, kNebulaTeal},
        {0.06, 0.72, 0.42, 13, kNebulaRose},
        {0.42, 0.20, 0.38, 11, kNebulaBlue},
    };
    for (const auto& cloud : clouds) {
        glow(painter, QPointF(w * cloud.x, h * cloud.y),
             qMax(w, h) * cloud.spread, cloud.color, cloud.peak / 100.0);
    }

    // A handful of brighter stars behind the band, so the field has structure
    // before the fine dust goes on. These sit in the corners: a bright glow in
    // the middle of the page would compete with the search field.
    for (int i = 0; i < 9; ++i) {
        // Alternate between the far corners, biased away from the centre band
        // the wordmark and the search field sit in.
        const bool right = (i % 2) == 0;
        const qreal x = w * (right ? 0.80 + nebula.bounded(180) / 1000.0
                                   : 0.02 + nebula.bounded(180) / 1000.0);
        const qreal y = h * (nebula.bounded(1000) / 1000.0);
        glow(painter, QPointF(x, y), qMax(w, h) * 0.16, kNebulaBlue, 0.05);
    }

    milkyWay(painter, size, nebula);

    // The star field itself: many faint, some brighter, a few with flares.
    QRandomGenerator random(0x59A0);
    const qreal area = w * h;
    const int faint = static_cast<int>(qBound(420.0, area / 900.0, 5200.0));
    const int medium = static_cast<int>(qBound(90.0, area / 9000.0, 900.0));
    const int bright = 14;

    for (int i = 0; i < faint; ++i) {
        const qreal x = random.bounded(100000) / 100000.0 * w;
        const qreal y = random.bounded(100000) / 100000.0 * h;
        const qreal brightness = 0.22 + random.bounded(46) / 100.0;
        const QColor tone = random.bounded(100) < 22 ? kStarWarm : kStarCool;
        star(painter, QPointF(x, y), 0.7, brightness, tone, false);
    }
    for (int i = 0; i < medium; ++i) {
        const qreal x = random.bounded(100000) / 100000.0 * w;
        const qreal y = random.bounded(100000) / 100000.0 * h;
        const qreal brightness = 0.55 + random.bounded(35) / 100.0;
        const QColor tone = random.bounded(100) < 30 ? kStarWarm : kStarCool;
        star(painter, QPointF(x, y), 1.05, brightness, tone, false);
    }
    for (int i = 0; i < bright; ++i) {
        const qreal x = random.bounded(100000) / 100000.0 * w;
        const qreal y = random.bounded(100000) / 100000.0 * h;
        const QColor tone = random.bounded(100) < 40 ? kStarWarm : kStarCool;
        star(painter, QPointF(x, y), 1.5, 1.0, tone, true);
    }

    // Vignette: the corners fall away so the search field sits in the brightest
    // part of the picture.
    QRadialGradient vignette(QPointF(w * 0.5, h * 0.44), qMax(w, h) * 0.78);
    vignette.setColorAt(0.0, QColor(0x00, 0x00, 0x00, 0));
    vignette.setColorAt(0.62, QColor(0x00, 0x00, 0x00, 26));
    vignette.setColorAt(1.0, QColor(0x00, 0x00, 0x00, 120));
    painter.setPen(Qt::NoPen);
    painter.setBrush(vignette);
    painter.drawRect(QRectF(0, 0, w, h));
}

}  // namespace

void NightSky::paint(QPainter& painter, const QSize& logicalSize, qreal devicePixelRatio)
{
    if (logicalSize.isEmpty() || devicePixelRatio <= 0.0) {
        return;
    }

    // One cached sky for the whole process. The home page is the only user, and
    // it repaints on every resize, hover and focus change - regenerating a few
    // thousand stars each time is what makes this kind of background stutter.
    static QPixmap cache;
    const QSize target(qMax(1, qRound(logicalSize.width() * devicePixelRatio)),
                       qMax(1, qRound(logicalSize.height() * devicePixelRatio)));
    if (cache.size() != target || cache.devicePixelRatio() != devicePixelRatio) {
        render(&cache, target, devicePixelRatio);
    }
    // The source rect is in the pixmap's own (device) pixels, the destination in
    // the painter's logical units; at a 1.25 or 1.5 scale factor those are not
    // the same size, and passing cache.rect() would draw the sky too large.
    painter.drawPixmap(QRectF(QPointF(0, 0), logicalSize), cache, QRectF(cache.rect()));
}

}  // namespace yozora
