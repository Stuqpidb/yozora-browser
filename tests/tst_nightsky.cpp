// SPDX-License-Identifier: MIT
//
// The night sky is generated rather than loaded, so the two ways it can go
// wrong are silent: the cache can hand back a stale or empty pixmap, and a
// background that is only "dark blue" looks fine until it is compared against
// what it is supposed to contain. Both are checked here against the pixels.

#include "core/NightSky.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace yozora;

namespace {

QImage render(qreal dpr = 1.0)
{
    QImage image(900, 600, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    NightSky::paint(painter, image.size(), dpr);
    painter.end();
    return image;
}

// Mean luminance of a region. The mean rather than the peak, because a single
// bright star would otherwise decide the comparison.
qreal meanLuminance(const QImage& image, const QRect& area)
{
    qreal total = 0.0;
    int samples = 0;
    for (int y = area.top(); y <= area.bottom(); ++y) {
        for (int x = area.left(); x <= area.right(); ++x) {
            const QColor c = image.pixelColor(x, y);
            total += 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();
            ++samples;
        }
    }
    return samples > 0 ? total / samples : 0.0;
}

}  // namespace

class TestNightSky : public QObject {
    Q_OBJECT

private slots:
    void paintsSomethingOpaque();
    void hasStars();
    void theMoonIsInTheCorner();
    void isDeterministic();
    void survivesAResize();
};

void TestNightSky::paintsSomethingOpaque()
{
    const QImage image = render();
    // A sky with alpha holes would let the window chrome show through.
    QCOMPARE(image.pixelColor(450, 300).alpha(), 255);
    QCOMPARE(image.pixelColor(4, 4).alpha(), 255);
}

void TestNightSky::hasStars()
{
    const QImage image = render();
    // Count pixels that are markedly brighter than the base gradient: each one
    // is a star, a nebula peak or the moon.
    int bright = 0;
    for (int y = 0; y < image.height(); y += 2) {
        for (int x = 0; x < image.width(); x += 2) {
            if (image.pixelColor(x, y).value() > 110) {
                ++bright;
            }
        }
    }
    QVERIFY2(bright > 60, qPrintable(QStringLiteral("only %1 bright samples").arg(bright)));
}

void TestNightSky::theMoonIsInTheCorner()
{
    const QImage image = render();
    // The moon sits in the upper right and has to be the brightest thing on the
    // page, or it reads as a smudge. Compared on the mean over the disc, since
    // the surrounding halo would otherwise carry the comparison on its own.
    const qreal moon = meanLuminance(image, QRect(700, 60, 68, 68));
    const qreal elsewhere = meanLuminance(image, QRect(40, 300, 240, 240));
    QVERIFY2(moon > 0.55, qPrintable(QStringLiteral("moon mean %1").arg(moon)));
    QVERIFY2(moon > elsewhere * 3.0,
             qPrintable(QStringLiteral("moon %1 vs background %2").arg(moon).arg(elsewhere)));
}

void TestNightSky::isDeterministic()
{
    // Same size twice in a row must be pixel-identical: the cache is keyed on
    // size, and a "random" star field would make the window flicker on repaint.
    const QImage first = render();
    const QImage second = render();
    QCOMPARE(first, second);
}

void TestNightSky::survivesAResize()
{
    // The cache is reused across sizes; this is the case where a stale pixmap
    // would leave part of the page unpainted.
    const QImage small = render();
    QImage large(1600, 1000, QImage::Format_ARGB32_Premultiplied);
    large.fill(Qt::transparent);
    QPainter painter(&large);
    NightSky::paint(painter, large.size(), 1.0);
    painter.end();

    QCOMPARE(large.pixelColor(800, 500).alpha(), 255);
    QCOMPARE(large.pixelColor(large.width() - 3, 3).alpha(), 255);
    // Back to the original size, and it must still be the same picture.
    QCOMPARE(render(), small);
}

QTEST_MAIN(TestNightSky)
#include "tst_nightsky.moc"
