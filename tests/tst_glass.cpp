// SPDX-License-Identifier: MIT
//
// The glass helpers are drawn by hand, so a silent geometry mistake would just
// look like "the design is a bit off". These tests check the pixels.

#include "core/Glass.h"

#include <QImage>
#include <QPainter>
#include <QTest>

using namespace yozora;

class TestGlass : public QObject {
    Q_OBJECT

private slots:
    void squircleFillsItsCentre();
    void squircleFillsTheWholeSquare();
    void squircleStaysInsideTheRect();
    void panelIsTranslucent();
    void chipIsRounded();
    void shadowStaysInsideItsMargin();
};

void TestGlass::squircleFillsItsCentre()
{
    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::red);
    const QPainterPath path = Glass::squircle(QRectF(0, 0, 96, 96), 26);
    painter.drawPath(path);
    painter.end();

    QCOMPARE(image.pixelColor(48, 48).alpha(), 255);
    QCOMPARE(image.pixelColor(48, 48).red(), 255);
}

void TestGlass::squircleFillsTheWholeSquare()
{
    QImage image(96, 96, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(Qt::red);
    const QRectF rect(0, 0, 96, 96);
    const QPainterPath path = Glass::squircle(rect, 26);
    painter.drawPath(path);
    painter.end();

    // A superellipse reaches the extremes of its own bounding box on the
    // axes. Checking the box rather than the passed rect is what catches the
    // sign bug that collapses the curve into a single quadrant. The sample
    // points sit one pixel inside the boundary, because the outermost pixel of
    // a shape is only half covered by antialiasing.
    const QRectF box = path.boundingRect();
    const int cx = qRound(box.center().x());
    const int cy = qRound(box.center().y());
    QVERIFY(image.pixelColor(cx, qRound(box.top()) + 1).alpha() > 128);
    QVERIFY(image.pixelColor(cx, qRound(box.bottom()) - 1).alpha() > 128);
    QVERIFY(image.pixelColor(qRound(box.left()) + 1, cy).alpha() > 128);
    QVERIFY(image.pixelColor(qRound(box.right()) - 1, cy).alpha() > 128);

    // And the shape is smaller than the rect it was given, which is what makes
    // it an icon rather than a full-bleed panel.
    QVERIFY(box.width() < rect.width());
    QVERIFY(image.pixelColor(2, 2).alpha() < 128);
}

void TestGlass::squircleStaysInsideTheRect()
{
    const QRectF rect(10, 20, 60, 40);
    const QPainterPath path = Glass::squircle(rect, 18);
    QVERIFY(path.boundingRect().left() >= rect.left() - 0.5);
    QVERIFY(path.boundingRect().right() <= rect.right() + 0.5);
    QVERIFY(path.boundingRect().top() >= rect.top() - 0.5);
    QVERIFY(path.boundingRect().bottom() <= rect.bottom() + 0.5);
}

void TestGlass::panelIsTranslucent()
{
    const auto recipe = Glass::recipe(18);
    QVERIFY(recipe.fill.alpha() < 255);
    QVERIFY(recipe.fill.alpha() > 0);

    QImage image(200, 80, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    Glass::paintPanel(painter, QRectF(0, 0, 200, 80), recipe);
    painter.end();

    const QColor middle = image.pixelColor(100, 40);
    QVERIFY2(middle.alpha() > 0, "the panel body must be painted");
    QVERIFY2(middle.alpha() < 255, "glass must stay translucent");
}

void TestGlass::chipIsRounded()
{
    const auto recipe = Glass::recipe(18);
    QImage image(120, 40, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    Glass::paintChip(painter, QRectF(0, 0, 120, 40), recipe);
    painter.end();

    // A pill has transparent corners and an opaque middle.
    QCOMPARE(image.pixelColor(1, 1).alpha(), 0);
    QVERIFY(image.pixelColor(60, 20).alpha() > 0);
}

void TestGlass::shadowStaysInsideItsMargin()
{
    // A surface that clips its own shadow looks pressed into the page, so the
    // blur has to land entirely inside the margin the caller reserves.
    const auto recipe = Glass::recipe(18);
    const QRectF pill(Glass::shadowMargin(), Glass::shadowMargin(),
                       200 - 2 * Glass::shadowMargin(), 60 - 2 * Glass::shadowMargin());

    QImage image(200, 60, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    Glass::paintShadow(painter, pill, recipe, 1.0);
    painter.end();

    // The blur lands just outside the pill...
    QVERIFY2(image.pixelColor(100, 11).alpha() > 0, "no shadow above the pill");
    QVERIFY2(image.pixelColor(100, 50).alpha() > 0, "no shadow below the pill");
    // ...and nothing is cut off at the border of the widget.
    QCOMPARE(image.pixelColor(0, 30).alpha(), 0);
    QCOMPARE(image.pixelColor(199, 30).alpha(), 0);
    QCOMPARE(image.pixelColor(100, 0).alpha(), 0);
    QCOMPARE(image.pixelColor(100, 59).alpha(), 0);
}

QTEST_MAIN(TestGlass)
#include "tst_glass.moc"
