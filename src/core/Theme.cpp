// SPDX-License-Identifier: MIT
#include "core/Theme.h"

#include <QApplication>
#include <QColor>
#include <QFontDatabase>
#include <QPalette>
#include <QStringList>

namespace yozora {

Theme::Colors Theme::darkColors()
{
    return {
        /*background*/ QStringLiteral("#0d1017"),
        /*surface*/ QStringLiteral("#151a23"),
        /*surfaceHover*/ QStringLiteral("#1e2531"),
        /*surfaceActive*/ QStringLiteral("#273040"),
        /*tabActive*/ QStringLiteral("#1b2230"),
        /*tabInactive*/ QStringLiteral("#0f131a"),
        /*border*/ QStringLiteral("#252d3a"),
        /*text*/ QStringLiteral("#e6eaf2"),
        /*textMuted*/ QStringLiteral("#8b95a7"),
        /*accent*/ QStringLiteral("#6ea8fe"),
        /*accentText*/ QStringLiteral("#0b0e14"),
        /*danger*/ QStringLiteral("#e05561"),
        /*field*/ QStringLiteral("#1a212c"),
        /*fieldText*/ QStringLiteral("#e6eaf2"),
        /*shadow*/ QStringLiteral("rgba(0, 0, 0, 0.55)"),
    };
}

Theme::Colors Theme::lightColors()
{
    return {
        /*background*/ QStringLiteral("#f4f6fa"),
        /*surface*/ QStringLiteral("#ffffff"),
        /*surfaceHover*/ QStringLiteral("#eceff5"),
        /*surfaceActive*/ QStringLiteral("#dfe4ee"),
        /*tabActive*/ QStringLiteral("#ffffff"),
        /*tabInactive*/ QStringLiteral("#e8ebf2"),
        /*border*/ QStringLiteral("#d3d9e5"),
        /*text*/ QStringLiteral("#1a1f2b"),
        /*textMuted*/ QStringLiteral("#5d6675"),
        /*accent*/ QStringLiteral("#2f6bd8"),
        /*accentText*/ QStringLiteral("#ffffff"),
        /*danger*/ QStringLiteral("#c9372f"),
        /*field*/ QStringLiteral("#ffffff"),
        /*fieldText*/ QStringLiteral("#1a1f2b"),
        /*shadow*/ QStringLiteral("rgba(20, 25, 40, 0.18)"),
    };
}

QString Theme::fontFamily()
{
    const QStringList families = {
        QStringLiteral("Segoe UI Variable Text"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Inter"),
        QStringLiteral("Noto Sans"),
    };
    const QStringList available = QFontDatabase::families();
    for (const auto& family : families) {
        if (available.contains(family, Qt::CaseInsensitive)) {
            return family;
        }
    }
    return QStringLiteral("sans-serif");
}

void Theme::apply(bool dark)
{
    const auto colors = dark ? darkColors() : lightColors();

    QPalette palette;
    const QColor window(colors.background);
    const QColor base(colors.surface);
    const QColor text(colors.text);
    const QColor disabled(colors.textMuted);
    const QColor highlight(colors.accent);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, colors.surfaceHover);
    palette.setColor(QPalette::ToolTipBase, colors.surface);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, base);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, colors.danger);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, colors.accentText);
    palette.setColor(QPalette::PlaceholderText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);

    qApp->setPalette(palette);
    qApp->setStyleSheet(styleSheet(dark));
}

QString Theme::styleSheet(bool dark)
{
    const auto c = dark ? darkColors() : lightColors();
    return QStringLiteral(R"(
QWidget {
    color: %TEXT%;
    font-family: "%FONT%";
    font-size: 13px;
}

/* ---- Dialogs ------------------------------------------------------------ */
QDialog {
    background: %BACKGROUND%;
}

QGroupBox {
    background: %SURFACE%;
    border: 1px solid %BORDER%;
    border-radius: 10px;
    margin-top: 14px;
    padding: 16px 14px 14px 14px;
}

QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 6px;
    color: %TEXT%;
}

QLabel#hintLabel { color: %TEXT_MUTED%; }

QLabel#privateBadge {
    color: %ACCENT_TEXT%;
    background: %ACCENT%;
    border-radius: 7px;
    padding: 2px 8px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.08em;
}

QPushButton {
    background: %SURFACE_HOVER%;
    border: 1px solid %BORDER%;
    border-radius: 7px;
    padding: 6px 16px;
}

QPushButton:hover { background: %SURFACE_ACTIVE%; }
QPushButton:default { border-color: %ACCENT%; }
QPushButton:disabled { color: %TEXT_MUTED%; background: %SURFACE%; }

QToolTip {
    background: %SURFACE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    padding: 4px 8px;
}

/* ---- Tab strip ------------------------------------------------------- */
QTabWidget::pane {
    border: none;
    background: %BACKGROUND%;
}

QTabBar {
    qproperty-drawBase: 0;
    background: transparent;
}

QTabBar::tab {
    background: %TAB_INACTIVE%;
    color: %TEXT_MUTED%;
    border: 1px solid transparent;
    border-top-left-radius: 8px;
    border-top-right-radius: 8px;
    min-width: 90px;
    max-width: 240px;
    height: 30px;
    padding: 0 10px;
    margin-right: 2px;
}

QTabBar::tab:hover {
    background: %SURFACE_HOVER%;
    color: %TEXT%;
}

QTabBar::tab:selected {
    background: %TAB_ACTIVE%;
    color: %TEXT%;
    border-color: %BORDER%;
    border-bottom-color: %TAB_ACTIVE%;
}

QTabBar::close-button {
    subcontrol-position: right;
}

QTabBar::close-button:hover {
    background: %DANGER%;
    border-radius: 7px;
}

QTabBar::close-button:pressed {
    background: %DANGER%;
    border-radius: 7px;
}

/* ---- Tool buttons ---------------------------------------------------- */
QToolButton {
    background: transparent;
    border: none;
    border-radius: 6px;
    color: %TEXT%;
    padding: 5px;
}

QToolButton:hover {
    background: %SURFACE_HOVER%;
}

QToolButton:pressed {
    background: %SURFACE_ACTIVE%;
}

QToolButton:disabled {
    color: %TEXT_MUTED%;
    background: transparent;
}

QToolButton#newTabButton:hover {
    background: %SURFACE_HOVER%;
    color: %ACCENT%;
}

/* ---- Address bar ----------------------------------------------------- */
QLineEdit {
    background: %FIELD%;
    color: %FIELD_TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 16px;
    padding: 6px 12px;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
}

QLineEdit:hover {
    border-color: %SURFACE_ACTIVE%;
}

QLineEdit:focus {
    border-color: %ACCENT%;
}

/* ---- Menu ------------------------------------------------------------ */
QMenu {
    background: %SURFACE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    padding: 4px;
}

QMenu::item {
    padding: 6px 26px 6px 12px;
    border-radius: 5px;
}

QMenu::item:selected {
    background: %SURFACE_HOVER%;
}

QMenu::separator {
    height: 1px;
    background: %BORDER%;
    margin: 4px 8px;
}

/* ---- Scrollbars ------------------------------------------------------ */
QScrollBar:vertical {
    background: transparent;
    width: 10px;
    margin: 0;
}

QScrollBar::handle:vertical {
    background: %BORDER%;
    border-radius: 5px;
    min-height: 30px;
}

QScrollBar::handle:vertical:hover {
    background: %TEXT_MUTED%;
}

QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical,
QScrollBar::sub-page:vertical {
    height: 0;
    background: none;
}
)")
        .replace(QStringLiteral("%TEXT%"), c.text)
        .replace(QStringLiteral("%TEXT_MUTED%"), c.textMuted)
        .replace(QStringLiteral("%BACKGROUND%"), c.background)
        .replace(QStringLiteral("%SURFACE%"), c.surface)
        .replace(QStringLiteral("%SURFACE_HOVER%"), c.surfaceHover)
        .replace(QStringLiteral("%SURFACE_ACTIVE%"), c.surfaceActive)
        .replace(QStringLiteral("%TAB_ACTIVE%"), c.tabActive)
        .replace(QStringLiteral("%TAB_INACTIVE%"), c.tabInactive)
        .replace(QStringLiteral("%BORDER%"), c.border)
        .replace(QStringLiteral("%ACCENT%"), c.accent)
        .replace(QStringLiteral("%ACCENT_TEXT%"), c.accentText)
        .replace(QStringLiteral("%DANGER%"), c.danger)
        .replace(QStringLiteral("%FIELD%"), c.field)
        .replace(QStringLiteral("%FIELD_TEXT%"), c.fieldText)
        .replace(QStringLiteral("%FONT%"), fontFamily());
}

QString Theme::htmlStyle(bool dark)
{
    const auto c = dark ? darkColors() : lightColors();
    return QStringLiteral(R"(
:root {
  --bg: %BG%;
  --surface: %SURFACE%;
  --border: %BORDER%;
  --text: %TEXT%;
  --muted: %TEXT_MUTED%;
  --accent: %ACCENT%;
  --accent-text: %ACCENT_TEXT%;
}
* { box-sizing: border-box; }
html, body {
  margin: 0;
  padding: 0;
  height: 100%;
  background: var(--bg);
  color: var(--text);
  font-family: "%FONT%", "Segoe UI", system-ui, sans-serif;
  -webkit-font-smoothing: antialiased;
}
a { color: var(--accent); text-decoration: none; }
a:hover { text-decoration: underline; }
)")
        .replace(QStringLiteral("%BG%"), c.background)
        .replace(QStringLiteral("%SURFACE%"), c.surface)
        .replace(QStringLiteral("%BORDER%"), c.border)
        .replace(QStringLiteral("%TEXT%"), c.text)
        .replace(QStringLiteral("%TEXT_MUTED%"), c.textMuted)
        .replace(QStringLiteral("%ACCENT%"), c.accent)
        .replace(QStringLiteral("%ACCENT_TEXT%"), c.accentText)
        .replace(QStringLiteral("%FONT%"), fontFamily());
}

}  // namespace yozora
