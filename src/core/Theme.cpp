// SPDX-License-Identifier: MIT
#include "core/Theme.h"

#include <QApplication>
#include <QColor>
#include <QFontDatabase>
#include <QPalette>
#include <QStringList>

namespace yozora {

namespace {
// Set once by apply(); read by every custom-painted widget.
bool g_dark = true;
}  // namespace

bool Theme::isDark()
{
    return g_dark;
}

void Theme::setDark(bool dark)
{
    g_dark = dark;
}

Theme::Colors Theme::darkColors()
{
    return {
        /*background*/ QStringLiteral("#070a12"),
        /*surface*/ QStringLiteral("#0d111b"),
        /*surfaceHover*/ QStringLiteral("#161d2b"),
        /*surfaceActive*/ QStringLiteral("#1f2939"),
        /*tabActive*/ QStringLiteral("#18202f"),
        /*tabInactive*/ QStringLiteral("rgba(255, 255, 255, 0.03)"),
        /*border*/ QStringLiteral("rgba(255, 255, 255, 0.07)"),
        /*text*/ QStringLiteral("#e8edf7"),
        /*textMuted*/ QStringLiteral("#8b97ad"),
        /*accent*/ QStringLiteral("#6ea8fe"),
        /*accent2*/ QStringLiteral("#8f7bff"),
        /*accentText*/ QStringLiteral("#06090f"),
        /*danger*/ QStringLiteral("#e05561"),
        /*field*/ QStringLiteral("rgba(255, 255, 255, 0.05)"),
        /*fieldText*/ QStringLiteral("#e8edf7"),
        /*shadow*/ QStringLiteral("rgba(0, 0, 0, 0.6)"),
        /*rail*/ QStringLiteral("rgba(255, 255, 255, 0.02)"),
        /*card*/ QStringLiteral("rgba(255, 255, 255, 0.055)"),
        /*cardBorder*/ QStringLiteral("rgba(255, 255, 255, 0.09)"),
        /*chip*/ QStringLiteral("rgba(255, 255, 255, 0.07)"),
    };
}

Theme::Colors Theme::lightColors()
{
    return {
        /*background*/ QStringLiteral("#eef1f7"),
        /*surface*/ QStringLiteral("#f7f9fc"),
        /*surfaceHover*/ QStringLiteral("rgba(15, 25, 50, 0.055)"),
        /*surfaceActive*/ QStringLiteral("rgba(15, 25, 50, 0.09)"),
        /*tabActive*/ QStringLiteral("rgba(255, 255, 255, 0.92)"),
        /*tabInactive*/ QStringLiteral("rgba(255, 255, 255, 0.45)"),
        /*border*/ QStringLiteral("rgba(15, 25, 50, 0.10)"),
        /*text*/ QStringLiteral("#131822"),
        /*textMuted*/ QStringLiteral("#5c6678"),
        /*accent*/ QStringLiteral("#2f6bd8"),
        /*accent2*/ QStringLiteral("#6b53d6"),
        /*accentText*/ QStringLiteral("#ffffff"),
        /*danger*/ QStringLiteral("#c9372f"),
        /*field*/ QStringLiteral("rgba(255, 255, 255, 0.85)"),
        /*fieldText*/ QStringLiteral("#131822"),
        /*shadow*/ QStringLiteral("rgba(16, 22, 40, 0.16)"),
        /*rail*/ QStringLiteral("rgba(255, 255, 255, 0.55)"),
        /*card*/ QStringLiteral("rgba(255, 255, 255, 0.66)"),
        /*cardBorder*/ QStringLiteral("rgba(255, 255, 255, 0.75)"),
        /*chip*/ QStringLiteral("rgba(15, 25, 50, 0.05)"),
    };
}

QString Theme::accentGradient(const Colors& c)
{
    return QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2)")
        .arg(c.accent, c.accent2);
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
    g_dark = dark;
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

QMainWindow, QDialog { background: %BACKGROUND%; }

/* ---- Left navigation rail ------------------------------------------- */
/* A soft top-to-bottom sheen so the rail reads as a glass panel rather than
   as a flat column of colour. */
QWidget#sideBar {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 %GLASS_RAIL%, stop:1 %GLASS_RAIL_EDGE%);
    border-right: 1px solid %BORDER%;
}
QToolButton#railButton {
    background: transparent;
    border: none;
    border-radius: 11px;
    color: %TEXT_MUTED%;
    padding: 0;
}
QToolButton#railButton:hover { background: %SURFACE_HOVER%; color: %TEXT%; }
QToolButton#railButton:checked { background: %SURFACE_ACTIVE%; color: %TEXT%; }

/* ---- Top chrome ------------------------------------------------------ */
/* The same sheen as the rail: lighter where the light falls, so the bar looks
   like a sheet of glass lying on the window. */
QWidget#navigationBar {
    background: qlineargradient(x1:0, y1:0, x2:0, y2:1,
        stop:0 %GLASS_TOP%, stop:1 %GLASS_TOP_EDGE%);
    border-bottom: 1px solid %BORDER%;
}
QLabel#brandLabel { color: %TEXT%; font-weight: 600; letter-spacing: 0.04em; }
QLabel#privateBadge {
    color: %ACCENT_TEXT%;
    background: %ACCENT_GRADIENT%;
    border-radius: 8px;
    padding: 2px 9px;
    font-size: 10px;
    font-weight: 700;
    letter-spacing: 0.08em;
}
QLabel#navMessage { color: %TEXT_MUTED%; }

QToolButton#navButton {
    background: transparent;
    border: none;
    border-radius: 15px;
    color: %TEXT%;
    padding: 0;
}
QToolButton#navButton:hover { background: %SURFACE_HOVER%; }
QToolButton#navButton:pressed { background: %SURFACE_ACTIVE%; }
QToolButton#navButton:disabled { color: %TEXT_MUTED%; }

/* ---- Address bar ----------------------------------------------------- */
/* The pill itself is painted in GlassField::paintEvent() as glass, so the
   style sheet must leave the background alone. */
GlassField#addressBar {
    background: transparent;
    border: none;
}

QProgressBar#pageProgress { background: transparent; border: none; }
QProgressBar#pageProgress::chunk { background: %ACCENT_GRADIENT%; border-radius: 1px; }

/* ---- Tabs ------------------------------------------------------------ */
QTabWidget::pane { border: none; background: %BACKGROUND%; }
QTabBar { qproperty-drawBase: 0; background: transparent; }
QTabBar::tab {
    background: %TAB_INACTIVE%;
    color: %TEXT_MUTED%;
    border: 1px solid transparent;
    border-radius: 10px;
    min-width: 118px;
    max-width: 240px;
    height: 30px;
    padding: 0 12px;
    margin: 7px 4px 0 0;
}
QTabBar::tab:hover { background: %SURFACE_HOVER%; color: %TEXT%; }
QTabBar::tab:selected {
    background: %TAB_ACTIVE%;
    color: %TEXT%;
    border-color: %BORDER%;
}
QTabBar::close-button {
    subcontrol-position: right;
    border-radius: 7px;
}
QTabBar::close-button:hover { background: %DANGER%; }

/* ---- Generic controls ------------------------------------------------ */
QToolButton {
    background: transparent;
    border: none;
    border-radius: 9px;
    color: %TEXT%;
    padding: 4px;
}
QToolButton:hover { background: %SURFACE_HOVER%; }
QToolButton:pressed { background: %SURFACE_ACTIVE%; }
QToolButton:disabled { color: %TEXT_MUTED%; }

QPushButton {
    background: %SURFACE_HOVER%;
    border: 1px solid %BORDER%;
    border-radius: 10px;
    padding: 7px 16px;
    color: %TEXT%;
}
QPushButton:hover { background: %SURFACE_ACTIVE%; }
QPushButton:disabled { color: %TEXT_MUTED%; background: %SURFACE%; }
QPushButton:default {
    background: %ACCENT_GRADIENT%;
    border: 1px solid transparent;
    color: %ACCENT_TEXT%;
}

QLineEdit {
    background: %FIELD%;
    color: %FIELD_TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 10px;
    padding: 7px 12px;
    selection-background-color: %ACCENT%;
    selection-color: %ACCENT_TEXT%;
}
QLineEdit:focus { border-color: %ACCENT%; }

QComboBox {
    background: %FIELD%;
    color: %FIELD_TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 10px;
    padding: 6px 12px;
}
QComboBox:hover { border-color: %SURFACE_ACTIVE%; }
QComboBox::drop-down { border: none; width: 20px; }
QComboBox QAbstractItemView {
    background: %SURFACE%;
    border: 1px solid %BORDER%;
    border-radius: 8px;
    selection-background-color: %SURFACE_HOVER%;
    color: %TEXT%;
}

QCheckBox, QRadioButton { spacing: 8px; color: %TEXT%; }

QListWidget {
    background: %FIELD%;
    border: 1px solid %BORDER%;
    border-radius: 12px;
    padding: 4px;
}

/* ---- Dialogs --------------------------------------------------------- */
QGroupBox {
    background: %SURFACE%;
    border: 1px solid %BORDER%;
    border-radius: 16px;
    margin-top: 16px;
    padding: 18px 16px 14px 16px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 14px;
    padding: 0 6px;
    color: %TEXT%;
    font-weight: 600;
}
QLabel#hintLabel { color: %TEXT_MUTED%; }

/* ---- Menu ------------------------------------------------------------ */
QMenu {
    background: %SURFACE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 12px;
    padding: 6px;
}
QMenu::item { padding: 7px 26px 7px 12px; border-radius: 8px; }
QMenu::item:selected { background: %SURFACE_HOVER%; }
QMenu::separator { height: 1px; background: %BORDER%; margin: 5px 8px; }

QToolTip {
    background: %SURFACE%;
    color: %TEXT%;
    border: 1px solid %BORDER%;
    border-radius: 6px;
    padding: 5px 9px;
}

/* ---- Scrollbars ------------------------------------------------------ */
QScrollBar:vertical { background: transparent; width: 10px; margin: 2px; }
QScrollBar::handle:vertical { background: %SURFACE_ACTIVE%; border-radius: 5px; min-height: 34px; }
QScrollBar::handle:vertical:hover { background: %TEXT_MUTED%; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { height: 0; background: none; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 2px; }
QScrollBar::handle:horizontal { background: %SURFACE_ACTIVE%; border-radius: 5px; min-width: 34px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal,
QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { width: 0; background: none; }

/* ---- Home page / dashboard ------------------------------------------ */
QWidget#homePage { background: transparent; }
QScrollArea#homeScroll { background: transparent; border: none; }
QScrollArea#homeScroll > QWidget > QWidget { background: transparent; }

/* HomeWidget::paintEvent() draws the glass card, including its drag and
   drop-target states. */
QFrame#homeCard { background: transparent; border: none; }
QFrame#homeCard[hero="true"] { background: transparent; }
QLabel#cardTitle { color: %TEXT%; font-weight: 600; }
QLabel#cardSubtle { color: %TEXT_MUTED%; }
QToolButton#cardMenu {
    background: transparent; border: none; border-radius: 8px;
    color: %TEXT_MUTED%; padding: 0;
}
QToolButton#cardMenu:hover { background: %SURFACE_HOVER%; color: %TEXT%; }

/* GlassField paints its own surface and icons; the editor inside it only has
   to supply the text, so it must not draw a background or a frame of its own. */
QLineEdit#glassFieldEditor {
    background: transparent;
    border: none;
    color: %TEXT%;
    selection-background-color: %ACCENT%;
    selection-color: %TEXT_ON_ACCENT%;
    font-size: 13px;
}
GlassField#heroSearch QLineEdit#glassFieldEditor { font-size: 15px; }

QPushButton#chip {
    background: %CHIP%;
    border: 1px solid %CARD_BORDER%;
    border-radius: 15px;
    padding: 6px 15px;
    color: %TEXT%;
}
QPushButton#chip:hover { background: %SURFACE_HOVER%; }

QToolButton#siteTile {
    background: %SURFACE%;
    border: 1px solid %CARD_BORDER%;
    border-radius: 15px;
    color: %TEXT%;
    padding: 8px 4px;
    font-size: 12px;
}
QToolButton#siteTile:hover { background: %SURFACE_HOVER%; }

QLabel#heroWordmark { color: %TEXT%; }

QPushButton#listRow {
    background: transparent;
    border: none;
    border-radius: 9px;
    padding: 7px 10px;
    text-align: left;
    color: %TEXT%;
    font-size: 12px;
}
QPushButton#listRow:hover { background: %SURFACE_HOVER%; }

QToolButton#cardAction {
    color: %ACCENT%;
    border-radius: 8px;
    padding: 2px 10px;
    font-size: 12px;
}
QToolButton#cardAction:hover { background: %SURFACE_HOVER%; }
QLabel#statsValue { color: %TEXT%; font-weight: 600; }
QLabel#privacyGlyph { color: %ACCENT%; }
QLabel#clockTime { color: %TEXT%; font-size: 30px; font-weight: 300; background: transparent; }
QLabel#clockDate { color: %TEXT_MUTED%; font-size: 12px; background: transparent; }

QListWidget::item { padding: 7px 10px; border-radius: 8px; color: %TEXT%; }
QListWidget::item:hover { background: %SURFACE_HOVER%; }
QListWidget::item:selected { background: %SURFACE_ACTIVE%; color: %TEXT%; }

QListWidget#settingsNav {
    background: %RAIL%;
    border: none;
    border-right: 1px solid %BORDER%;
    border-radius: 0;
    padding: 16px 12px;
    font-size: 13px;
}
QListWidget#settingsNav::item {
    padding: 10px 12px;
    border-radius: 10px;
    color: %TEXT_MUTED%;
    margin: 3px 0;
}
QListWidget#settingsNav::item:hover { background: %SURFACE_HOVER%; color: %TEXT%; }
QListWidget#settingsNav::item:selected { background: %SURFACE_ACTIVE%; color: %TEXT%; }
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
        .replace(QStringLiteral("%ACCENT_GRADIENT%"), accentGradient(c))
        .replace(QStringLiteral("%DANGER%"), c.danger)
        .replace(QStringLiteral("%FIELD%"), c.field)
        .replace(QStringLiteral("%FIELD_TEXT%"), c.fieldText)
        .replace(QStringLiteral("%RAIL%"), c.rail)
        .replace(QStringLiteral("%GLASS_TOP%"),
                 dark ? QStringLiteral("rgba(255, 255, 255, 0.055)")
                      : QStringLiteral("rgba(255, 255, 255, 0.92)"))
        .replace(QStringLiteral("%GLASS_TOP_EDGE%"),
                 dark ? QStringLiteral("rgba(255, 255, 255, 0.018)")
                      : QStringLiteral("rgba(255, 255, 255, 0.68)"))
        .replace(QStringLiteral("%GLASS_RAIL%"),
                 dark ? QStringLiteral("rgba(255, 255, 255, 0.035)")
                      : QStringLiteral("rgba(255, 255, 255, 0.72)"))
        .replace(QStringLiteral("%GLASS_RAIL_EDGE%"),
                 dark ? QStringLiteral("rgba(255, 255, 255, 0.012)")
                      : QStringLiteral("rgba(255, 255, 255, 0.5)"))
        .replace(QStringLiteral("%CARD%"), c.card)
        .replace(QStringLiteral("%CARD_BORDER%"), c.cardBorder)
        .replace(QStringLiteral("%CHIP%"), c.chip)
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
