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
    void theCentreHasNoBrightBody();
    void hasNoLargeBrightBody();
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

void TestNightSky::theCentreHasNoBrightBody()
{
    // The search field and the wordmark sit in the middle of the page, so nothing
    // bright may be painted there. The corners cannot be used as the baseline
    // here: the vignette deliberately darkens them, which makes the centre the
    // brightest region of the whole sky by design. So this counts near-white
    // pixels in the middle instead - a moon would fill the whole area, whereas
    // stars contribute a handful.
    const QImage image = render();
    const QRect centre(300, 200, 300, 200);
    int bright = 0;
    for (int y = centre.top(); y <= centre.bottom(); ++y) {
        for (int x = centre.left(); x <= centre.right(); ++x) {
            if (image.pixelColor(x, y).value() > 225) {
                ++bright;
            }
        }
    }
    QVERIFY2(bright < 60, qPrintable(QStringLiteral("%1 near-white pixels in the centre")
                                         .arg(bright)));
    // Sanity check on the measurement itself: the stars are still there, just
    // not in a blob. A count of zero would also satisfy the assert above.
    QVERIFY2(meanLuminance(image, centre) > 0.02,
             "the centre is not painted at all");
}

void TestNightSky::hasNoLargeBrightBody()
{
    // A disc the size of the old moon would show up as a dense block of very
    // bright pixels. The brightest single object allowed here is a star.
    const QImage image = render();
    int bright = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixelColor(x, y).value() > 225) {
                ++bright;
            }
        }
    }
    // 14 flared stars with a couple of pixels each, and nothing else.
    QVERIFY2(bright < 900, qPrintable(QStringLiteral("%1 near-white pixels").arg(bright)));
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
