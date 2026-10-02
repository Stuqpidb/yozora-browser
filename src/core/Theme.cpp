// SPDX-License-Identifier: MIT
#include "core/Theme.h"

#include "app/AppPaths.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFontDatabase>
#include <QPalette>
#include <QStringList>

namespace yozora {

namespace {

// The bundled typefaces. installer/make_fonts.py bakes the weights the
// interface uses out of the upstream variable fonts; see LICENSES/ for the
// licence of each family.
const QStringList kTextFaces = {
    QStringLiteral("Inter-Regular"),
    QStringLiteral("Inter-Medium"),
    QStringLiteral("Inter-SemiBold"),
    QStringLiteral("Inter-Bold"),
};
const QStringList kDisplayFaces = {
    QStringLiteral("SpaceGrotesk-Medium"),
    QStringLiteral("SpaceGrotesk-Bold"),
};

// Picks the first family that actually made it into the font database, so a
// failed font load degrades to the system face instead of a blank interface.
QString firstAvailable(const QStringList& wanted, const QStringList& fallbacks)
{
    const QStringList available = QFontDatabase::families();
    for (const QString& family : wanted) {
        if (available.contains(family, Qt::CaseInsensitive)) {
            return family;
        }
    }
    for (const QString& family : fallbacks) {
        if (available.contains(family, Qt::CaseInsensitive)) {
            return family;
        }
    }
    return QStringLiteral("sans-serif");
}

}  // namespace

Theme::Colors Theme::colors()
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

QString Theme::accentGradient(const Colors& c)
{
    return QStringLiteral("qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 %1, stop:1 %2)")
        .arg(c.accent, c.accent2);
}

void Theme::loadFonts()
{
    static bool loaded = false;
    if (loaded) {
        return;
    }
    loaded = true;
    AppPaths::ensureResourcesLoaded();
    // addApplicationFont() is handed the bytes rather than a path: the fonts
    // live in the executable's resource bundle, and the file overload has to be
    // told about the size or it will read a resource as if it were on disk.
    for (const QStringList& faces : {kTextFaces, kDisplayFaces}) {
        for (const QString& face : faces) {
            QFile file(QStringLiteral(":/fonts/%1.ttf").arg(face));
            if (file.open(QIODevice::ReadOnly)) {
                QFontDatabase::addApplicationFont(file.readAll());
            }
        }
    }
}

QString Theme::fontFamily()
{
    return firstAvailable({QStringLiteral("Inter")},
                          {QStringLiteral("Segoe UI Variable Text"), QStringLiteral("Segoe UI")});
}

QString Theme::displayFamily()
{
    return firstAvailable({QStringLiteral("Space Grotesk")},
                          {QStringLiteral("Segoe UI Variable Display"), QStringLiteral("Segoe UI")});
}

void Theme::apply()
{
    loadFonts();

    const auto c = colors();

    QPalette palette;
    const QColor window(c.background);
    const QColor base(c.surface);
    const QColor text(c.text);
    const QColor disabled(c.textMuted);
    const QColor highlight(c.accent);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, c.surfaceHover);
    palette.setColor(QPalette::ToolTipBase, c.surface);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, base);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, c.danger);
    palette.setColor(QPalette::Link, highlight);
    palette.setColor(QPalette::Highlight, highlight);
    palette.setColor(QPalette::HighlightedText, c.accentText);
    palette.setColor(QPalette::PlaceholderText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);

    // A hinting and spacing pass on the bundled face: without it Qt picks the
    // bitmap strike at small sizes and the text looks heavier than the design.
    QFont font = QFont(fontFamily());
    font.setHintingPreference(QFont::PreferFullHinting);
    font.setStyleStrategy(QFont::PreferAntialias);
    qApp->setFont(font);
    qApp->setPalette(palette);
    qApp->setStyleSheet(styleSheet());
}

QString Theme::styleSheet()
{
    const auto c = colors();
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
QLabel#brandLabel {
    color: %TEXT%;
    font-family: "%DISPLAY_FONT%";
    font-weight: 600;
    letter-spacing: 0.04em;
}
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

/* ---- History / bookmarks window --------------------------------------- */
/* Three bands: a header with the title, the count and the filter box, the list
   itself, and a footer with the destructive action and Close. The bands are
   separated by hairlines rather than by boxes, so the window reads as one
   surface instead of three stacked widgets. */
QWidget#libraryHeader { background: %SURFACE%; border-bottom: 1px solid %BORDER%; }
QWidget#libraryFooter { background: %SURFACE%; border-top: 1px solid %BORDER%; }
QLabel#dialogTitle {
    color: %TEXT%;
    font-family: "%DISPLAY_FONT%";
    font-size: 17px;
    font-weight: 600;
}
QLabel#dialogSubtitle { color: %TEXT_MUTED%; font-size: 12px; }

QLineEdit#searchBox {
    background: %FIELD%;
    border: 1px solid %BORDER%;
    border-radius: 12px;
    padding: 8px 12px;
    color: %TEXT%;
}
QLineEdit#searchBox:focus { border-color: %ACCENT%; }

QListWidget#libraryList {
    background: transparent;
    border: none;
    padding: 6px 8px;
    outline: none;
}
/* The row widget fills the item, so the selection has to be drawn by the item
   itself: a stylesheet background on the widget would sit under the labels. */
QListWidget#libraryList::item { border-radius: 12px; }
QListWidget#libraryList::item:selected { background: %SURFACE_HOVER%; }
QListWidget#libraryList::item:hover { background: %SURFACE_HOVER%; }
QListWidget#libraryList::item:selected:hover { background: %SURFACE_ACTIVE%; }

QLabel#rowTitle { color: %TEXT%; font-size: 13px; background: transparent; }
QLabel#rowSubtle { color: %TEXT_MUTED%; font-size: 11px; background: transparent; }
QLabel#rowTime { color: %TEXT_MUTED%; font-size: 11px; background: transparent; }

/* A button that looks like text until it is hovered: the only action in the
   footer that destroys something, and it should not compete with Close. */
QPushButton#quietButton {
    background: transparent;
    border: 1px solid transparent;
    color: %TEXT_MUTED%;
    padding: 6px 12px;
}
QPushButton#quietButton:hover {
    background: %SURFACE_HOVER%;
    border-color: %BORDER%;
    color: %DANGER%;
}
QPushButton#quietButton:disabled { color: %TEXT_MUTED%; background: transparent; }

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

/* ---- Home page --------------------------------------------------------- */
/* The page paints the night sky itself, so nothing here may have a
   background of its own. */
QWidget#homePage { background: transparent; }

QLabel#heroWordmark {
    color: %TEXT%;
    background: transparent;
    font-family: "%DISPLAY_FONT%";
}
QLabel#heroTagline { color: %TEXT_MUTED%; background: transparent; }
QLabel#pinsTitle {
    color: %TEXT_MUTED%;
    background: transparent;
    font-size: 11px;
    letter-spacing: 0.16em;
}

/* Pinned sites stand straight on the sky: no card, no frame, just the icon
   and its label, with a glass pill appearing only under the pointer. */
QToolButton#siteTile {
    background: transparent;
    border: none;
    border-radius: 16px;
    color: %TEXT%;
    padding: 8px 4px;
    font-size: 12px;
}
QToolButton#siteTile:hover {
    background: %SURFACE_HOVER%;
    border: 1px solid %CARD_BORDER%;
}

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
/* The focus ring is drawn by GlassField itself, so the editor must not add a
   second one of its own: two rings a pixel apart read as a printing error. */
GlassField QLineEdit#glassFieldEditor:focus,
GlassField QLineEdit#glassFieldEditor:focus:hover { border: none; }

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

QListWidget::item { padding: 7px 10px; border-radius: 8px; color: %TEXT%; }
QListWidget::item:hover { background: %SURFACE_HOVER%; }
QListWidget::item:selected { background: %SURFACE_ACTIVE%; color: %TEXT%; }

/* ---- Settings navigation ---------------------------------------------- */
/* A branded column: a wordmark band on top, the section rows below. The rows
   are painted by SettingsNavDelegate (see SettingsDialog.cpp), so the list
   itself stays transparent and only the delegate draws the icon, the label and
   the highlight. This is what makes the settings navigation match the rail. */
QWidget#settingsNavColumn {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 %GLASS_RAIL%, stop:1 %GLASS_RAIL_EDGE%);
    border-right: 1px solid %BORDER%;
}
QWidget#settingsBrand { background: transparent; border-bottom: 1px solid %BORDER%; }
QLabel#settingsBrandWord {
    color: %TEXT%;
    font-family: "%DISPLAY_FONT%";
    font-size: 19px;
    font-weight: 700;
    letter-spacing: 0.01em;
}
QLabel#settingsBrandCaption {
    color: %TEXT_MUTED%;
    font-size: 10px;
    font-weight: 600;
    letter-spacing: 0.24em;
}
QListWidget#settingsNav {
    background: transparent;
    border: none;
    outline: none;
    padding: 10px 6px;
}
/* The delegate owns the row layout, so the generic list item padding has to be
   cancelled here or it would inset the icon and the text a second time. */
QListWidget#settingsNav::item {
    background: transparent;
    border: none;
    padding: 0;
    margin: 0;
}
QListWidget#settingsNav::item:hover,
QListWidget#settingsNav::item:selected { background: transparent; }

/* ---- Settings pages --------------------------------------------------- */
/* Each section is a header band plus a scrolling body. The cards inside are the
   QGroupBoxes built by SettingsDialog. */
QScrollArea#settingsScroll { background: transparent; border: none; }
QScrollArea#settingsScroll > QWidget > QWidget { background: transparent; }

QGroupBox#settingsCard {
    background: %CARD%;
    border: 1px solid %CARD_BORDER%;
    border-radius: 16px;
    margin-top: 0;
    padding: 0;
}
QLabel#cardHeading { color: %TEXT%; font-weight: 600; }
QLabel#cardNote { color: %TEXT_MUTED%; font-size: 12px; }

/* A card's own title, the way QGroupBox draws one by default, has to be pulled
   back to match the rest of the interface. */
/* The cards are untitled: the band above them carries the section name, so a
   second title inside each card would repeat it one level down. */
QGroupBox {
    background: %CARD%;
    border: 1px solid %CARD_BORDER%;
    border-radius: 16px;
    margin-top: 0;
    padding: 20px 20px 20px 20px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 16px;
    padding: 0 8px 0 0;
    color: %TEXT%;
    font-weight: 600;
}
QCheckBox, QRadioButton { padding: 3px 0; }
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
        .replace(QStringLiteral("%GLASS_TOP%"), QStringLiteral("rgba(255, 255, 255, 0.055)"))
        .replace(QStringLiteral("%GLASS_TOP_EDGE%"), QStringLiteral("rgba(255, 255, 255, 0.018)"))
        .replace(QStringLiteral("%GLASS_RAIL%"), QStringLiteral("rgba(255, 255, 255, 0.035)"))
        .replace(QStringLiteral("%GLASS_RAIL_EDGE%"), QStringLiteral("rgba(255, 255, 255, 0.012)"))
        .replace(QStringLiteral("%CARD%"), c.card)
        .replace(QStringLiteral("%CARD_BORDER%"), c.cardBorder)
        .replace(QStringLiteral("%CHIP%"), c.chip)
        .replace(QStringLiteral("%FONT%"), fontFamily())
        .replace(QStringLiteral("%DISPLAY_FONT%"), displayFamily());
}

QString Theme::htmlStyle()
{
    const auto c = colors();
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
        .replace(QStringLiteral("%FONT%"), fontFamily())
        .replace(QStringLiteral("%DISPLAY_FONT%"), displayFamily());
}

}  // namespace yozora
