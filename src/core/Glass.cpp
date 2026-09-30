// SPDX-License-Identifier: MIT
#include "core/Glass.h"

#include <QColor>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QWidget>
#include <QWindow>
#include <QtMath>

#if defined(Q_OS_WIN)
#    define WIN32_LEAN_AND_MEAN
#    include <windows.h>
#    include <dwmapi.h>
// DwmSetWindowAttribute and the undocumented AccentPolicy hook. Declared here
// rather than pulled from a newer SDK so the build works on any toolchain.
#    ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#        define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#    endif
#    ifndef DWMWA_SYSTEMBACKDROP_TYPE
#        define DWMWA_SYSTEMBACKDROP_TYPE 38
#    endif
#    ifndef DWMWA_MICA_EFFECT
#        define DWMWA_MICA_EFFECT 1029
#    endif
#endif

namespace yozora {

namespace {

bool g_backdropActive = false;

// Cached film of noise, drawn once and reused. Regenerating it per repaint is
// the classic way to make a "glass" panel flicker, so it must be static.
QPixmap grainTile()
{
    static QPixmap tile;
    if (!tile.isNull()) {
        return tile;
    }
    tile = QPixmap(64, 64);
    tile.fill(Qt::transparent);
    QPainter painter(&tile);
    painter.setRenderHint(QPainter::Antialiasing, false);
    int state = 0x13579b;
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 64; ++x) {
            // A tiny deterministic LCG keeps the pattern stable between runs.
            state = (state * 1103515245 + 12345) & 0x7fffffff;
            const int level = (state >> 16) & 0xff;
            const int alpha = 6 + (level % 10);
            painter.setPen(QColor(255, 255, 255, alpha));
            painter.drawPoint(x, y);
        }
    }
    painter.end();
    return tile;
}

QColor withAlpha(const QColor& color, qreal alpha)
{
    QColor result = color;
    result.setAlphaF(qBound(0.0, color.alphaF() * alpha, 1.0));
    return result;
}

}  // namespace

Glass::Recipe Glass::recipe(qreal radius)
{
    Recipe r;
    r.fill = QColor(255, 255, 255, 16);
    r.fillTop = QColor(255, 255, 255, 30);
    r.stroke = QColor(255, 255, 255, 34);
    r.highlight = QColor(255, 255, 255, 70);
    r.shadow = QColor(0, 0, 0, 130);
    r.radius = radius;
    r.borderWidth = 1.0;
    r.grain = true;
    return r;
}

QPainterPath Glass::squircle(const QRectF& rect, qreal radius)
{
    // A superellipse with exponent 4: the continuous-curvature shape iOS uses
    // for app icons. A plain rounded rect looks obviously like a web button;
    // this one does not.
    const qreal n = 4.0;
    const qreal a = qMax<qreal>(0.5, radius);
    QPainterPath path;
    const int steps = 96;
    for (int i = 0; i <= steps; ++i) {
        const qreal t = (M_PI * 2.0 * i) / steps;
        const qreal ct = std::cos(t);
        const qreal st = std::sin(t);
        // The sign has to survive: pow(|cos|, 2/n) is always positive, so
        // without copysign the whole curve collapses into one quadrant.
        const qreal x = std::copysign(std::pow(std::abs(ct), 2.0 / n), ct)
            * qMin(a, rect.width() / 2.0);
        const qreal y = std::copysign(std::pow(std::abs(st), 2.0 / n), st)
            * qMin(a, rect.height() / 2.0);
        const QPointF point(rect.center().x() + x, rect.center().y() + y);
        if (i == 0) {
            path.moveTo(point);
        } else {
            path.lineTo(point);
        }
    }
    path.closeSubpath();
    return path;
}

void Glass::paintShadow(QPainter& painter, const QRectF& rect, const Recipe& recipe, qreal opacity)
{
    Q_UNUSED(recipe)
    QColor shadow = QColor(0, 0, 0, 96);
    if (opacity < 1.0) {
        shadow.setAlphaF(shadow.alphaF() * opacity);
    }
    // A few stacked, increasingly large rounded rects approximate a blur far
    // better than one hard offset rect. The spread stays inside shadowMargin()
    // so a widget that reserves that margin never has its shadow cut off at the
    // edges - which is exactly what made the start-page search look dented.
    constexpr int kLayers = 9;
    constexpr qreal kSpread = 13.5;
    painter.setPen(Qt::NoPen);
    for (int i = kLayers; i >= 1; --i) {
        const qreal spread = kSpread * i / kLayers;
        QColor layer = shadow;
        layer.setAlphaF(shadow.alphaF() / kLayers * opacity);
        painter.setBrush(layer);
        // Wider than tall: the blur has to stay inside shadowMargin() on every
        // side, which is what leaves the lower edge free for the direction the
        // light comes from.
        painter.drawRoundedRect(rect.adjusted(-spread, -spread * 0.45, spread, spread),
                                rect.height() / 2 + spread, rect.height() / 2 + spread);
    }
}

void Glass::paintPanel(QPainter& painter, const QRectF& rect, const Recipe& recipe, qreal opacity)
{
    const QRectF body = rect.adjusted(0.5, 0.5, -0.5, -0.5);

    QLinearGradient fill(body.topLeft(), body.bottomLeft());
    fill.setColorAt(0.0, withAlpha(recipe.fillTop, opacity));
    fill.setColorAt(0.55, withAlpha(recipe.fill, opacity));
    fill.setColorAt(1.0, withAlpha(recipe.fill, opacity * 0.92));

    painter.setPen(Qt::NoPen);
    painter.setBrush(fill);
    painter.drawRoundedRect(body, recipe.radius, recipe.radius);

    paintTopHighlight(painter, body, recipe, opacity);

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(withAlpha(recipe.stroke, opacity), recipe.borderWidth));
    painter.drawRoundedRect(body, recipe.radius, recipe.radius);

    if (recipe.grain) {
        paintGrain(painter, body, QColor(255, 255, 255), 0.028 * opacity);
    }
}

void Glass::paintChip(QPainter& painter, const QRectF& rect, const Recipe& recipe, qreal opacity)
{
    const QRectF body = rect.adjusted(0.5, 0.5, -0.5, -0.5);
    painter.setPen(Qt::NoPen);
    painter.setBrush(withAlpha(recipe.fill, 0.8 * opacity));
    painter.drawRoundedRect(body, body.height() / 2, body.height() / 2);
    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(withAlpha(recipe.stroke, 0.7 * opacity), 1.0));
    painter.drawRoundedRect(body, body.height() / 2, body.height() / 2);
}

void Glass::paintTopHighlight(QPainter& painter, const QRectF& rect, const Recipe& recipe,
                              qreal opacity)
{
    // Light falls from above, so the top edge catches a thin bright line while
    // the bottom edge fades into shadow. This single detail is most of what
    // makes a flat fill read as glass.
    const qreal radius = recipe.radius;
    QLinearGradient top(rect.topLeft(), rect.topRight());
    top.setColorAt(0.0, withAlpha(recipe.highlight, 0.0 * opacity));
    top.setColorAt(0.18, withAlpha(recipe.highlight, 0.85 * opacity));
    top.setColorAt(0.82, withAlpha(recipe.highlight, 0.85 * opacity));
    top.setColorAt(1.0, withAlpha(recipe.highlight, 0.0 * opacity));

    painter.save();
    QPainterPath clip;
    clip.addRoundedRect(rect, radius, radius);
    painter.setClipPath(clip);
    painter.setPen(QPen(top, 1.2));
    painter.drawLine(QPointF(rect.left() + 2, rect.top() + 1.0),
                     QPointF(rect.right() - 2, rect.top() + 1.0));
    painter.restore();
}

void Glass::paintGrain(QPainter& painter, const QRectF& rect, const QColor& tint, qreal opacity)
{
    painter.save();
    QPainterPath clip;
    clip.addRoundedRect(rect, rect.height() / 2, rect.height() / 2);
    painter.setClipPath(clip);
    painter.setOpacity(qBound(0.0, opacity, 1.0));
    painter.fillRect(rect, tint);
    painter.drawTiledPixmap(rect.toRect(), grainTile());
    painter.restore();
}

#if defined(Q_OS_WIN)

namespace {

enum AccentState {
    AccentDisabled = 0,
    AccentEnableGradient = 1,
    AccentEnableTransparentGradient = 2,
    AccentEnableBlurBehind = 3,
    AccentEnableAcrylicBlurBehind = 4,
};

enum AccentFlags {
    AccentFlagNone = 0,
    AccentFlagDrawLeftBorder = 0x20,
};

struct AccentPolicy {
    DWORD accentState = AccentDisabled;
    DWORD accentFlags = AccentFlagNone;
    DWORD gradientColor = 0x00a8a8a8;  // BGR
    DWORD animationId = 0;
};

struct WindowCompositionAttributeData {
    DWORD attribute;
    void* data;
    SIZE_T sizeOfData;
};

using SetWindowCompositionAttributeFn = BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*);

}  // namespace

#endif  // Q_OS_WIN

void Glass::applyWindowBackdrop(QWidget* window)
{
    g_backdropActive = false;
    if (!window) {
        return;
    }
    QWindow* handle = window->windowHandle();
    if (!handle) {
        // A native window may not exist until the widget is shown; the caller
        // invokes this again from showEvent, so failing here is fine.
        return;
    }

#if defined(Q_OS_WIN)
    const HWND hwnd = reinterpret_cast<HWND>(handle->winId());
    if (!hwnd) {
        return;
    }

    // Yozora is a dark-only product, so the native title bar always follows.
    const BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    // Windows 11 22H2+: the real system material.
    constexpr DWORD backdropMica = 2;      // DWMSBT_MAINWINDOW
    constexpr DWORD backdropMicaAlt = 3;   // DWMSBT_TRANSIENTWINDOW (windowed apps)
    constexpr DWORD backdropAcrylic = 4;   // DWMSBT_ACRYLICBLURBEHIND
    const HRESULT micaResult =
        DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdropMicaAlt, sizeof(backdropMicaAlt));
    if (SUCCEEDED(micaResult) && micaResult != E_INVALIDARG) {
        g_backdropActive = true;
        return;
    }
    // Older Windows 11 builds only knew the undocumented Mica attribute.
    const BOOL micaEnabled = TRUE;
    if (SUCCEEDED(DwmSetWindowAttribute(hwnd, DWMWA_MICA_EFFECT, &micaEnabled, sizeof(micaEnabled)))
        && micaEnabled) {
        g_backdropActive = true;
        return;
    }
    Q_UNUSED(backdropMica)
    Q_UNUSED(backdropAcrylic)

    // Windows 10: the AccentPolicy blur. Same visual idea, older API.
    static SetWindowCompositionAttributeFn setComposition = [] {
        HMODULE user32 = GetModuleHandleW(L"user32.dll");
        return user32 ? reinterpret_cast<SetWindowCompositionAttributeFn>(
                            GetProcAddress(user32, "SetWindowCompositionAttribute"))
                      : nullptr;
    }();
    if (setComposition) {
        AccentPolicy policy;
        policy.accentState = AccentEnableAcrylicBlurBehind;
        policy.accentFlags = AccentFlagNone;
        policy.gradientColor = 0x00101010;  // BGR
        WindowCompositionAttributeData data;
        data.attribute = 19;  // WCA_ACCENT_POLICY
        data.data = &policy;
        data.sizeOfData = sizeof(policy);
        if (setComposition(hwnd, &data)) {
            g_backdropActive = true;
        }
    }
#endif
}

bool Glass::windowBackdropActive()
{
    return g_backdropActive;
}

}  // namespace yozora
