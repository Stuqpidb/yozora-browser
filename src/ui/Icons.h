// SPDX-License-Identifier: MIT
#pragma once

#include <QColor>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRectF>
#include <QtMath>

// One icon set for the whole application.
//
// The toolbar used to draw Unicode symbols at 15pt and the rail at 21pt, but
// the application style sheet sets font-size: 13px and a style-sheet font size
// wins over setFont() in code - so every one of those glyphs rendered at 13px
// and looked broken. Drawing the shapes removes the font from the picture
// entirely and keeps the stroke weight identical everywhere.
namespace yozora::icons {

// The nominal size of every icon. The painters scale to whatever rect they are
// given, so a control can change size without changing its icons.
inline constexpr qreal kBox = 24.0;
inline constexpr qreal kStroke = 2.0;

enum class Shape {
    Back,
    Forward,
    Reload,
    Stop,
    Star,
    Menu,
    Plus,
    Close,
    Magnifier,
    ArrowRight,
    More,   // three dots: "more options"
    // The ad/tracker shield, used both in the address bar and as the Privacy
    // section glyph.
    Shield,
    // Section glyphs for the settings navigation. Kept in the same grid and
    // stroke as the toolbar icons so the two never look like different sets.
    Home,
    Download,
    Wheel,
    Database,
    Key,
    Info,
    // Window controls, drawn on the same 24x24 grid as everything else.
    Minimize,
    Maximize,
    Restore,
};

inline void stroke(QPainter& painter, const QPainterPath& path, const QColor& color,
                   qreal width = kStroke)
{
    QPen pen(color, width);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(path);
}

inline void fill(QPainter& painter, const QPainterPath& path, const QColor& color)
{
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPath(path);
}

inline void draw(QPainter& painter, Shape shape, const QRectF& box, const QColor& color,
                 qreal width = kStroke)
{
    // Icons are drawn on a 24x24 grid and mapped onto the target rect, so a
    // stroke of 2 grid units always looks the same relative to the glyph.
    const qreal scale = qMin(box.width(), box.height()) / kBox;
    painter.save();
    painter.translate(box.topLeft());
    painter.scale(scale, scale);
    const QRectF g(0, 0, kBox, kBox);
    const QPointF c = g.center();

    switch (shape) {
        case Shape::Back: {
            QPainterPath path;
            path.moveTo(19, 12);
            path.lineTo(6, 12);
            path.moveTo(11.5, 6.5);
            path.lineTo(5.5, 12);
            path.lineTo(11.5, 17.5);
            stroke(painter, path, color, width);
            break;
        }
        case Shape::Forward: {
            QPainterPath path;
            path.moveTo(5, 12);
            path.lineTo(18, 12);
            path.moveTo(12.5, 6.5);
            path.lineTo(18.5, 12);
            path.lineTo(12.5, 17.5);
            stroke(painter, path, color, width);
            break;
        }
        case Shape::Reload: {
            // Three quarters of a circle with an arrow head: the standard
            // refresh gesture, and far clearer than the old glyph.
            QPainterPath arc;
            arc.arcMoveTo(QRectF(3.5, 3.5, 17, 17), 40);
            arc.arcTo(QRectF(3.5, 3.5, 17, 17), 40, 285);
            stroke(painter, arc, color, width);
            QPainterPath head;
            head.moveTo(20.0, 6.2);
            head.lineTo(20.6, 12.4);
            head.lineTo(14.6, 10.2);
            stroke(painter, head, color, width);
            break;
        }
        case Shape::Stop: {
            QPainterPath box2;
            box2.addRoundedRect(QRectF(6, 6, 12, 12), 2.5, 2.5);
            fill(painter, box2, color);
            break;
        }
        case Shape::Star: {
            QPainterPath path;
            const int points = 5;
            for (int i = 0; i < points * 2; ++i) {
                const qreal angle = -M_PI / 2.0 + i * M_PI / points;
                const qreal r = (i % 2 == 0) ? 9.0 : 3.9;
                const QPointF p(c.x() + std::cos(angle) * r, c.y() + std::sin(angle) * r);
                if (i == 0) {
                    path.moveTo(p);
                } else {
                    path.lineTo(p);
                }
            }
            path.closeSubpath();
            fill(painter, path, color);
            break;
        }
        case Shape::Menu: {
            for (const qreal y : {7.0, 12.0, 17.0}) {
                QPainterPath line;
                line.moveTo(4, y);
                line.lineTo(20, y);
                stroke(painter, line, color, width);
            }
            break;
        }
        case Shape::Plus: {
            QPainterPath path;
            path.moveTo(12, 5);
            path.lineTo(12, 19);
            path.moveTo(5, 12);
            path.lineTo(19, 12);
            stroke(painter, path, color, width);
            break;
        }
        case Shape::Close: {
            QPainterPath path;
            path.moveTo(6.5, 6.5);
            path.lineTo(17.5, 17.5);
            path.moveTo(17.5, 6.5);
            path.lineTo(6.5, 17.5);
            stroke(painter, path, color, width);
            break;
        }
        case Shape::Magnifier: {
            QPainterPath ring;
            ring.addEllipse(QRectF(4, 4, 12, 12));
            stroke(painter, ring, color, width);
            QPainterPath handle;
            handle.moveTo(14.8, 14.8);
            handle.lineTo(20, 20);
            stroke(painter, handle, color, width);
            break;
        }
        case Shape::ArrowRight: {
            QPainterPath path;
            path.moveTo(4, 12);
            path.lineTo(19, 12);
            path.moveTo(13, 6);
            path.lineTo(19, 12);
            path.lineTo(13, 18);
            stroke(painter, path, color, width);
            break;
        }
        case Shape::More: {
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            for (const qreal x : {6.0, 12.0, 18.0}) {
                painter.drawEllipse(QPointF(x, 12), 1.8, 1.8);
            }
            break;
        }
        case Shape::Home: {
            QPainterPath roof;
            roof.moveTo(3.5, 11);
            roof.lineTo(12, 4);
            roof.lineTo(20.5, 11);
            stroke(painter, roof, color, width);
            QPainterPath body;
            body.moveTo(6, 10.2);
            body.lineTo(6, 20);
            body.lineTo(18, 20);
            body.lineTo(18, 10.2);
            stroke(painter, body, color, width);
            break;
        }
        case Shape::Download: {
            QPainterPath stem;
            stem.moveTo(12, 4);
            stem.lineTo(12, 15);
            stroke(painter, stem, color, width);
            QPainterPath head;
            head.moveTo(7.5, 10.5);
            head.lineTo(12, 15);
            head.lineTo(16.5, 10.5);
            stroke(painter, head, color, width);
            QPainterPath tray;
            tray.moveTo(5, 19.5);
            tray.lineTo(19, 19.5);
            stroke(painter, tray, color, width);
            break;
        }
        case Shape::Shield: {
            QPainterPath path;
            path.moveTo(12, 3.5);
            path.lineTo(19, 6.2);
            path.lineTo(19, 11.5);
            path.cubicTo(19, 16.4, 15.6, 19.5, 12, 20.7);
            path.cubicTo(8.4, 19.5, 5, 16.4, 5, 11.5);
            path.lineTo(5, 6.2);
            path.closeSubpath();
            stroke(painter, path, color, width);
            break;
        }
        case Shape::Wheel: {
            QPainterPath body;
            body.addRoundedRect(QRectF(6.5, 3.5, 11, 17), 5.5, 5.5);
            stroke(painter, body, color, width);
            QPainterPath line;
            line.moveTo(12, 7.5);
            line.lineTo(12, 10.3);
            stroke(painter, line, color, width);
            QPainterPath up;
            up.moveTo(9.9, 13.1);
            up.lineTo(12, 11);
            up.lineTo(14.1, 13.1);
            stroke(painter, up, color, width);
            QPainterPath down;
            down.moveTo(9.9, 16.7);
            down.lineTo(12, 18.8);
            down.lineTo(14.1, 16.7);
            stroke(painter, down, color, width);
            break;
        }
        case Shape::Database: {
            QPainterPath top;
            top.addEllipse(QRectF(5, 3.5, 14, 5));
            stroke(painter, top, color, width);
            QPainterPath sides;
            sides.moveTo(5, 6);
            sides.lineTo(5, 18);
            sides.moveTo(19, 6);
            sides.lineTo(19, 18);
            stroke(painter, sides, color, width);
            QPainterPath bottom;
            bottom.moveTo(5, 18);
            bottom.cubicTo(5, 20.8, 19, 20.8, 19, 18);
            stroke(painter, bottom, color, width);
            QPainterPath middle;
            middle.moveTo(5, 12);
            middle.cubicTo(5, 14.8, 19, 14.8, 19, 12);
            stroke(painter, middle, color, width);
            break;
        }
        case Shape::Key: {
            QPainterPath ring;
            ring.addEllipse(QRectF(3.5, 8, 8, 8));
            stroke(painter, ring, color, width);
            QPainterPath shaft;
            shaft.moveTo(11.5, 12);
            shaft.lineTo(20.5, 12);
            shaft.moveTo(17, 12);
            shaft.lineTo(17, 15.4);
            shaft.moveTo(20.5, 12);
            shaft.lineTo(20.5, 14.6);
            stroke(painter, shaft, color, width);
            break;
        }
        case Shape::Info: {
            QPainterPath ring;
            ring.addEllipse(QRectF(3.5, 3.5, 17, 17));
            stroke(painter, ring, color, width);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawEllipse(QPointF(12, 7.8), 1.25, 1.25);
            QPainterPath stem;
            stem.moveTo(12, 10.8);
            stem.lineTo(12, 16.8);
            stroke(painter, stem, color, width);
            break;
        }
        case Shape::Minimize: {
            QPainterPath line;
            line.moveTo(6, 12);
            line.lineTo(18, 12);
            stroke(painter, line, color, 1.6);
            break;
        }
        case Shape::Maximize: {
            QPainterPath square;
            square.addRoundedRect(QRectF(6.5, 6.5, 11, 11), 2, 2);
            stroke(painter, square, color, 1.6);
            break;
        }
        case Shape::Restore: {
            // Two overlapping squares: the "restore down" glyph.
            QPainterPath back;
            back.moveTo(8.5, 8.5);
            back.lineTo(8.5, 6.5);
            back.lineTo(17.5, 6.5);
            back.lineTo(17.5, 15.5);
            back.lineTo(15.5, 15.5);
            stroke(painter, back, color, 1.6);
            QPainterPath front;
            front.addRoundedRect(QRectF(6.5, 8.5, 9, 9), 2, 2);
            stroke(painter, front, color, 1.6);
            break;
        }
    }
    painter.restore();
}

// Renders a shape for the controls that only accept a QIcon: QToolButton,
// QPushButton and the rest. Drawing on a transparent pixmap keeps the same
// shapes and the same stroke weight as the hand painted ones.
inline QPixmap pixmap(Shape shape, int size, const QColor& color, qreal scale = 1.0)
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter painter(&pm);
    painter.setRenderHint(QPainter::Antialiasing, true);
    draw(painter, shape, QRectF(0.0, 0.0, size, size), color, scale);
    painter.end();
    return pm;
}

inline QIcon icon(Shape shape, int size, const QColor& color, qreal scale = 1.0)
{
    return QIcon(pixmap(shape, size, color, scale));
}

}  // namespace yozora::icons
