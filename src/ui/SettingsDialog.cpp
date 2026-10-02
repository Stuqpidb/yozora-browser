// SPDX-License-Identifier: MIT
#include "ui/SettingsDialog.h"

#include "core/Animation.h"
#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "core/Theme.h"
#include "ui/ClearBrowsingDataDialog.h"
#include "ui/Icons.h"
#include "utils/Version.h"
#include "web/WebProfile.h"

#include <QAbstractAnimation>
#include <QAbstractItemView>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFont>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QScrollArea>
#include <QShowEvent>
#include <QStackedWidget>
#include <QStyle>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace yozora {

namespace {

QFrame* separator()
{
    auto* line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setObjectName(QStringLiteral("separator"));
    return line;
}

QLabel* hint(const QString& text, QWidget* parent)
{
    auto* label = new QLabel(text, parent);
    label->setObjectName(QStringLiteral("hintLabel"));
    label->setWordWrap(true);
    return label;
}

// One row of the settings section list, painted by hand.
//
// A style sheet can colour a list row but it cannot put the icon, the label and
// the highlight on the same baseline. Drawing the row is what lets the settings
// navigation look like the left rail instead of a plain system list, and it is
// the same approach the rail already uses.
class SettingsNavDelegate : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const override
    {
        return {0, 42};
    }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override
    {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        const auto c = Theme::colors();
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const QRectF row = QRectF(option.rect).adjusted(8, 2, -8, -2);
        const QColor accent(c.accent);

        if (selected) {
            QColor fill = accent;
            fill.setAlpha(38);
            QColor line = accent;
            line.setAlpha(120);
            painter->setPen(QPen(line, 1.0));
            painter->setBrush(fill);
            painter->drawRoundedRect(row, 11, 11);

            // A short accent bar keeps the current section legible even where
            // the glass pill is faint against the background.
            painter->setPen(Qt::NoPen);
            painter->setBrush(accent);
            painter->drawRoundedRect(QRectF(row.left() + 5, row.center().y() - 9, 3.0, 18),
                                     1.5, 1.5);
        } else if (hovered) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(255, 255, 255, 16));
            painter->drawRoundedRect(row, 11, 11);
        }

        const QColor glyph = selected ? accent : QColor(c.textMuted);
        const QRectF iconBox(row.left() + 16, row.center().y() - 9.5, 19, 19);
        icons::draw(*painter, static_cast<icons::Shape>(index.data(Qt::UserRole).toInt()),
                    iconBox, glyph, 1.7);

        QFont font = option.font;
        font.setPixelSize(13);
        font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
        painter->setFont(font);
        painter->setPen(selected ? QColor(c.text) : QColor(c.textMuted));

        const QRectF text(row.left() + 48, row.top(), row.width() - 56, row.height());
        painter->drawText(text, Qt::AlignVCenter | Qt::AlignLeft,
                          index.data(Qt::DisplayRole).toString());

        painter->restore();
    }
};

}  // namespace

// A section: its title, the sentence under it, and the page's own widgets.
// Defined out of line here because the header only forward-declares it, and the
// constructor needs the full type to build the list of them.
struct SettingsDialog::Section {
    icons::Shape icon;
    QString title;
    QString subtitle;
    QWidget* page;
};

SettingsDialog::SettingsDialog(Settings* settings, WebProfile* profile, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
    , m_profile(profile)
{
    setWindowTitle(tr("Yozora Settings"));
    setMinimumSize(860, 620);

    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // The section list lives in a branded column: a wordmark band on top and
    // the sections below it, painted by SettingsNavDelegate. It was a bare
    // system list before, which read as a different application next to the
    // rail and the cards.
    auto* navColumn = new QWidget(this);
    navColumn->setObjectName(QStringLiteral("settingsNavColumn"));
    navColumn->setAttribute(Qt::WA_StyledBackground, true);
    navColumn->setFixedWidth(236);
    auto* navLayout = new QVBoxLayout(navColumn);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(0);

    auto* brand = new QWidget(navColumn);
    brand->setObjectName(QStringLiteral("settingsBrand"));
    brand->setAttribute(Qt::WA_StyledBackground, true);
    auto* brandLayout = new QVBoxLayout(brand);
    brandLayout->setContentsMargins(22, 20, 22, 15);
    brandLayout->setSpacing(1);
    auto* wordmark = new QLabel(tr("Yozora"), brand);
    wordmark->setObjectName(QStringLiteral("settingsBrandWord"));
    brandLayout->addWidget(wordmark);
    auto* brandCaption = new QLabel(tr("SETTINGS"), brand);
    brandCaption->setObjectName(QStringLiteral("settingsBrandCaption"));
    brandLayout->addWidget(brandCaption);
    navLayout->addWidget(brand);

    m_nav = new QListWidget(navColumn);
    m_nav->setObjectName(QStringLiteral("settingsNav"));
    m_nav->setItemDelegate(new SettingsNavDelegate(m_nav));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_nav->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // The section list has to be reachable from the keyboard. It used to be
    // NoFocus, which meant the only way to change section was the mouse - and
    // since the controls inside a page hold the focus, even Up/Down landed on a
    // combo box instead of the list.
    m_nav->setFocusPolicy(Qt::StrongFocus);
    m_nav->setFocusProxy(nullptr);
    m_nav->setMouseTracking(true);
    m_nav->viewport()->setMouseTracking(true);
    navLayout->addWidget(m_nav, 1);
    root->addWidget(navColumn);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto* stack = new QStackedWidget(right);
    const QList<Section> sections = {
        {icons::Shape::Magnifier, tr("Search"),
         tr("Which engine answers what you type in the address bar."), buildSearchSection()},
        {icons::Shape::Home, tr("Startup"), tr("What happens when the browser opens."),
         buildStartupSection()},
        {icons::Shape::Download, tr("Downloads"), tr("Where files are saved."),
         buildDownloadsSection()},
        {icons::Shape::Shield, tr("Privacy"),
         tr("The defaults here are the private ones. Everything that weakens privacy has to be "
            "turned on on purpose."),
         buildPrivacySection()},
        {icons::Shape::Wheel, tr("Scrolling"), tr("How the mouse wheel moves a page."),
         buildScrollingSection()},
        {icons::Shape::Database, tr("Data"), tr("What Yozora keeps on this machine."),
         buildDataSection()},
        {icons::Shape::Info, tr("About"), tr("Version and licences."), buildAboutSection()},
    };
    for (const Section& section : sections) {
        auto* item = new QListWidgetItem(section.title, m_nav);
        item->setData(Qt::UserRole, static_cast<int>(section.icon));
        stack->addWidget(wrapSection(section, stack));
    }
    connect(m_nav, &QListWidget::currentRowChanged, this, [this, stack](int row) {
        showSection(stack, row);
    });
    m_nav->setCurrentRow(0);
    m_pages = stack;

    rightLayout->addWidget(stack, 1);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    auto* buttonRow = new QHBoxLayout;
    buttonRow->setContentsMargins(20, 10, 20, 16);
    buttonRow->addStretch(1);
    buttonRow->addWidget(m_buttons);
    rightLayout->addLayout(buttonRow);

    root->addWidget(right, 1);

    // Settings apply immediately, the way desktop browsers behave.
    connect(m_searchEngine, &QComboBox::currentIndexChanged, this, [this](int) {
        updateCustomEngineEnabled();
        applyToSettings();
    });
    connect(m_customSearchName, &QLineEdit::editingFinished, this, &SettingsDialog::applyToSettings);
    connect(m_customSearchUrl, &QLineEdit::editingFinished, this, &SettingsDialog::applyToSettings);
    connect(m_homePage, &QLineEdit::editingFinished, this, &SettingsDialog::applyToSettings);
    connect(m_askWhereToSave, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_restoreSession, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_scrollMode, &QComboBox::currentIndexChanged, this, [this](int) {
        applyToSettings();
    });

    connect(m_blockThirdPartyCookies, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_keepCookies, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_blockAds, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_sendDnt, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_notifications, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_webrtcPolicy, &QComboBox::currentIndexChanged, this, [this](int) {
        applyToSettings();
    });

    loadFromSettings();
    updateCustomEngineEnabled();
    refreshPermissions();
}

void SettingsDialog::showEvent(QShowEvent* event)
{
    QDialog::showEvent(event);
    // The section list takes the keyboard on open. Without this the first Up or
    // Down the user presses goes to whatever control happens to hold the focus,
    // which on the Search page is the engine combo box - so arrowing through the
    // sections silently changed the search engine instead.
    m_nav->setFocus(Qt::OtherFocusReason);
}

QWidget* SettingsDialog::wrapSection(const Section& section, QWidget* parent)
{
    auto* wrapper = new QWidget(parent);

    auto* outer = new QVBoxLayout(wrapper);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // The section's own name as a band across the top. The cards below it are
    // deliberately unnamed: a heading per card on top of this one is two levels
    // of title saying the same thing, which is what made the old layout look
    // like a stack of boxes.
    auto* header = new QWidget(wrapper);
    header->setObjectName(QStringLiteral("sectionHeader"));
    auto* headerLayout = new QVBoxLayout(header);
    headerLayout->setContentsMargins(26, 18, 26, 14);
    headerLayout->setSpacing(3);

    auto* heading = new QLabel(section.title, header);
    heading->setObjectName(QStringLiteral("dialogTitle"));
    headerLayout->addWidget(heading);

    auto* subtitle = new QLabel(section.subtitle, header);
    subtitle->setObjectName(QStringLiteral("dialogSubtitle"));
    subtitle->setWordWrap(true);
    headerLayout->addWidget(subtitle);
    outer->addWidget(header);

    auto* scroll = new QScrollArea(wrapper);
    scroll->setObjectName(QStringLiteral("settingsScroll"));
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // The section's own margins are handled by its layout; the scroll area adds
    // none, so the cards line up with the header above them.
    scroll->setWidget(section.page);
    outer->addWidget(scroll, 1);

    return wrapper;
}

void SettingsDialog::showSection(QStackedWidget* stack, int row)
{
    if (row < 0 || row >= stack->count()) {
        return;
    }
    QWidget* incoming = stack->widget(row);
    if (stack->currentWidget() == incoming) {
        return;
    }

    // The switch is instant. A cross-fade looked like nothing happened at all
    // over a fraction of a second, and the fade machinery (a graphics effect on
    // a widget the stack owns) was more fragile than the effect is worth: the
    // section still read as the old one until the animation finished.
    stack->setCurrentWidget(incoming);
    // Any page that was mid-fade from an earlier rapid change keeps its effect
    // otherwise, and a page left at opacity 0 is an invisible section.
    for (int i = 0; i < stack->count(); ++i) {
        if (QGraphicsOpacityEffect* effect =
                qobject_cast<QGraphicsOpacityEffect*>(stack->widget(i)->graphicsEffect())) {
            delete effect;  // setGraphicsEffect() transferred ownership to Qt
        }
    }
}

QWidget* SettingsDialog::buildSearchSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* box = new QGroupBox(page);
    auto* form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);

    m_searchEngine = new QComboBox(box);
    for (const auto& engine : SearchEngines::builtin()) {
        m_searchEngine->addItem(engine.name, engine.id);
    }
    m_searchEngine->addItem(tr("Custom"), QString::fromLatin1(SearchEngines::kCustomId));
    form->addRow(tr("Default engine:"), m_searchEngine);

    m_customSearchName = new QLineEdit(box);
    m_customSearchName->setPlaceholderText(QStringLiteral("My search"));
    form->addRow(tr("Custom name:"), m_customSearchName);

    m_customSearchUrl = new QLineEdit(box);
    m_customSearchUrl->setPlaceholderText(QStringLiteral("https://example.com/search?q=%s"));
    form->addRow(tr("Custom URL:"), m_customSearchUrl);

    m_customSearchNote = hint(tr("The custom URL must contain %s where the query goes. "
                                 "Queries are sent straight to the chosen provider; Yozora "
                                 "never sees them."),
                              box);
    form->addRow(QString(), m_customSearchNote);

    layout->addWidget(box);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildStartupSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* box = new QGroupBox(page);
    auto* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);

    m_homePage = new QLineEdit(box);
    m_homePage->setPlaceholderText(QStringLiteral("about:yozora"));
    form->addRow(tr("Page opened in a new tab:"), m_homePage);
    form->addRow(QString(), hint(tr("Use about:yozora for the Yozora start page, or any address."),
                                 box));

    m_restoreSession = new QCheckBox(tr("Reopen the previous session on start"), page);

    layout->addWidget(box);
    layout->addWidget(m_restoreSession);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildDownloadsSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* box = new QGroupBox(page);
    auto* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->setHorizontalSpacing(20);
    form->setVerticalSpacing(12);

    auto* dirRow = new QWidget(box);
    auto* dirLayout = new QHBoxLayout(dirRow);
    dirLayout->setContentsMargins(0, 0, 0, 0);
    dirLayout->setSpacing(8);

    m_downloadDir = new QLineEdit(dirRow);
    m_downloadDir->setReadOnly(true);
    dirLayout->addWidget(m_downloadDir, 1);

    m_browseButton = new QPushButton(tr("Change..."), dirRow);
    connect(m_browseButton, &QPushButton::clicked, this, &SettingsDialog::chooseDownloadDirectory);
    dirLayout->addWidget(m_browseButton);

    form->addRow(tr("Save files to:"), dirRow);
    layout->addWidget(box);

    m_askWhereToSave = new QCheckBox(tr("Ask where to save each file"), page);
    layout->addWidget(m_askWhereToSave);
    layout->addWidget(hint(tr("Downloaded programs are never run automatically. Yozora asks "
                              "before opening a file that can execute code."),
                           page));
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildPrivacySection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* cookies = new QGroupBox(page);
    auto* cookiesLayout = new QVBoxLayout(cookies);
    m_blockThirdPartyCookies = new QCheckBox(tr("Block third-party cookies"), cookies);
    m_keepCookies = new QCheckBox(tr("Keep cookies when Yozora closes"), cookies);
    cookiesLayout->addWidget(m_blockThirdPartyCookies);
    cookiesLayout->addWidget(m_keepCookies);
    cookiesLayout->addWidget(hint(tr("Blocking third-party cookies stops many cross-site "
                                     "trackers. First-party cookies still work, so logins keep "
                                     "working."),
                                  cookies));
    layout->addWidget(cookies);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* tracking = new QGroupBox(page);
    auto* trackingLayout = new QVBoxLayout(tracking);
    m_blockAds = new QCheckBox(tr("Block ads and trackers on every site"), tracking);
    m_sendDnt = new QCheckBox(tr("Send \"Do Not Track\" and \"Global Privacy Control\" signals"),
                              tracking);
    trackingLayout->addWidget(m_blockAds);
    trackingLayout->addWidget(m_sendDnt);
    trackingLayout->addWidget(hint(tr("The ad and tracker lists ship with Yozora and are applied "
                                      "locally; nothing is ever fetched from Yozora's servers. "
                                      "Sites can be allowed one by one from the shield in the "
                                      "address bar."),
                                   tracking));
    layout->addWidget(tracking);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* notifications = new QGroupBox(page);
    auto* notificationsLayout = new QVBoxLayout(notifications);
    m_notifications = new QCheckBox(tr("Allow sites to ask to show notifications"), notifications);
    notificationsLayout->addWidget(m_notifications);
    notificationsLayout->addWidget(hint(tr("Sites still have to ask for permission. Turning this "
                                           "off refuses every notification request without "
                                           "asking."),
                                        notifications));
    layout->addWidget(notifications);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* network = new QGroupBox(page);
    auto* networkForm = new QFormLayout(network);
    networkForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    m_webrtcPolicy = new QComboBox(network);
    m_webrtcPolicy->addItem(tr("Default (Chromium decides)"));
    m_webrtcPolicy->addItem(tr("Hide local addresses (public interface only)"));
    m_webrtcPolicy->addItem(tr("Only through a proxy (may break calls)"));
    networkForm->addRow(tr("WebRTC routing:"), m_webrtcPolicy);
    networkForm->addRow(QString(),
                        hint(tr("A normal browser cannot hide your IP address from a site without "
                                "a proxy, VPN or Tor. This only limits how WebRTC shares network "
                                "addresses. It takes effect after a restart."),
                             network));
    layout->addWidget(network);

    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildScrollingSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* box = new QGroupBox(page);
    auto* scrollLayout = new QVBoxLayout(box);
    m_scrollMode = new QComboBox(box);
    m_scrollMode->addItem(tr("Fast (recommended)"),
                          static_cast<int>(Settings::ScrollMode::Fast));
    m_scrollMode->addItem(tr("Instant"), static_cast<int>(Settings::ScrollMode::Instant));
    m_scrollMode->addItem(tr("Smooth (engine)"),
                          static_cast<int>(Settings::ScrollMode::Smooth));
    scrollLayout->addWidget(m_scrollMode);
    scrollLayout->addWidget(hint(tr("\"Fast\" animates the wheel briefly: smooth and quick, "
                                    "without the engine's slow easing.\n\"Instant\" jumps "
                                    "straight to the new position.\n\"Smooth\" uses the "
                                    "engine's own animation."),
                                 box));
    layout->addWidget(box);

    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildDataSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* box = new QGroupBox(page);
    auto* boxLayout = new QVBoxLayout(box);

    m_storagePath = new QLabel(box);
    m_storagePath->setObjectName(QStringLiteral("hintLabel"));
    m_storagePath->setWordWrap(true);
    m_storagePath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    boxLayout->addWidget(m_storagePath);

    auto* clear = new QPushButton(tr("Clear browsing data..."), box);
    connect(clear, &QPushButton::clicked, this, [this] {
        if (!m_profile) {
            return;
        }
        ClearBrowsingDataDialog dialog(m_profile, this);
        dialog.exec();
        refreshPermissions();
    });
    boxLayout->addWidget(clear);
    boxLayout->addWidget(hint(tr("Everything stays on this computer. Yozora has no account, no "
                                 "sync and no telemetry."),
                              box));
    layout->addWidget(box);

    // Untitled on purpose: the section header above already says what this page is,
    // and a second heading inside it would say the same thing one level down.
    auto* permsBox = new QGroupBox(page);
    auto* permsLayout = new QVBoxLayout(permsBox);
    m_permissionList = new QListWidget(permsBox);
    m_permissionList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_permissionList->setMinimumHeight(120);
    permsLayout->addWidget(m_permissionList);

    auto* permsButtons = new QHBoxLayout;
    auto* removeSelected = new QPushButton(tr("Remove selected"), permsBox);
    connect(removeSelected, &QPushButton::clicked, this, [this] {
        if (!m_profile) {
            return;
        }
        const auto items = m_permissionList->selectedItems();
        for (auto* item : items) {
            const QString origin = item->data(Qt::UserRole).toString();
            const int typeId = item->data(Qt::UserRole + 1).toInt();
            m_profile->revokePermission(origin, typeId);
        }
        refreshPermissions();
    });
    permsButtons->addWidget(removeSelected);

    auto* clearAll = new QPushButton(tr("Clear all"), permsBox);
    connect(clearAll, &QPushButton::clicked, this, &SettingsDialog::clearAllPermissions);
    permsButtons->addWidget(clearAll);
    permsButtons->addStretch(1);
    permsLayout->addLayout(permsButtons);

    layout->addWidget(permsBox);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildAboutSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(26, 20, 26, 24);
    layout->setSpacing(12);

    m_versionLabel = new QLabel(page);
    m_versionLabel->setText(QStringLiteral("Yozora Browser %1").arg(
        QString::fromLatin1(kVersionString)));
    m_versionLabel->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    layout->addWidget(m_versionLabel);

    auto* engine = hint(tr("Rendering engine: Qt WebEngine (Chromium)\n"
                           "Privacy-focused. No telemetry, no tracking, no account.\n"
                           "Yozora is an independent project and is not affiliated with Google "
                           "or Mozilla."),
                        page);
    layout->addWidget(engine);
    layout->addSpacing(6);

    // Updates are a button rather than a menu item hidden three levels deep.
    // Nothing is fetched until this is pressed.
    auto* updates = new QGroupBox(page);
    auto* updatesLayout = new QVBoxLayout(updates);
    m_checkUpdates = new QPushButton(tr("Check for updates"), updates);
    m_checkUpdates->setCursor(Qt::PointingHandCursor);
    m_updateStatus = new QLabel(updates);
    m_updateStatus->setObjectName(QStringLiteral("hintLabel"));
    m_updateStatus->setWordWrap(true);
    updatesLayout->addWidget(m_checkUpdates);
    updatesLayout->addWidget(m_updateStatus);

    m_backgroundUpdates = new QCheckBox(tr("Check for updates in the background"), updates);
    updatesLayout->addWidget(m_backgroundUpdates);
    updatesLayout->addWidget(hint(tr("Off by default: Yozora does not contact the network "
                                     "unless you ask. When on, it checks once shortly after "
                                     "start and then occasionally, and only ever talks to "
                                     "GitHub Releases."),
                                  updates));
    connect(m_backgroundUpdates, &QCheckBox::toggled, this, [this](bool on) {
        if (m_settings && !m_loading) {
            m_settings->setBackgroundUpdates(on);
        }
    });

    layout->addWidget(updates);

    connect(m_checkUpdates, &QPushButton::clicked, this, [this] {
        m_checkUpdates->setEnabled(false);
        m_checkUpdates->setText(tr("Checking..."));
        m_updateStatus->setText(tr("Asking GitHub for the latest release..."));
        emit updateCheckRequested();
    });

    layout->addWidget(separator());
    layout->addStretch(1);
    return page;
}

void SettingsDialog::setUpdateCheckResult(const QString& text, bool failed)
{
    m_checkUpdates->setEnabled(true);
    m_checkUpdates->setText(tr("Check for updates"));
    m_updateStatus->setText(text);
    if (failed) {
        m_updateStatus->setObjectName(QStringLiteral("dialogSubtitle"));
    } else {
        m_updateStatus->setObjectName(QStringLiteral("hintLabel"));
    }
    // The status colour comes from the object name, so it has to be re-polished.
    m_updateStatus->style()->unpolish(m_updateStatus);
    m_updateStatus->style()->polish(m_updateStatus);
}

void SettingsDialog::loadFromSettings()
{
    m_loading = true;

    if (m_settings) {
        const int index = m_searchEngine->findData(m_settings->searchEngineId());
        m_searchEngine->setCurrentIndex(index >= 0 ? index : 0);
        m_customSearchName->setText(m_settings->customSearchEngineName());
        m_customSearchUrl->setText(m_settings->customSearchEngineUrl());

        m_homePage->setText(m_settings->homePage());
        m_downloadDir->setText(m_settings->downloadDirectory());
        m_askWhereToSave->setChecked(m_settings->askWhereToSave());
        m_restoreSession->setChecked(m_settings->restoreSessionOnStart());

        m_blockThirdPartyCookies->setChecked(m_settings->blockThirdPartyCookies());
        m_keepCookies->setChecked(m_settings->keepCookiesOnExit());
        m_blockAds->setChecked(m_settings->blockAds());
        m_sendDnt->setChecked(m_settings->sendDoNotTrack());
        m_notifications->setChecked(m_settings->notificationsEnabled());
        m_webrtcPolicy->setCurrentIndex(static_cast<int>(m_settings->webrtcPolicy()));

        m_scrollMode->setCurrentIndex(
            m_scrollMode->findData(static_cast<int>(m_settings->scrollMode())));

        m_backgroundUpdates->setChecked(m_settings->backgroundUpdates());
    }

    if (m_profile) {
        m_storagePath->setText(tr("Profile folder: %1").arg(m_profile->storagePath()));
    }

    m_loading = false;
}

void SettingsDialog::applyToSettings()
{
    if (m_loading || !m_settings) {
        return;
    }
    m_settings->setSearchEngineId(m_searchEngine->currentData().toString());
    m_settings->setCustomSearchEngineName(m_customSearchName->text().trimmed());
    m_settings->setCustomSearchEngineUrl(m_customSearchUrl->text().trimmed());
    m_settings->setHomePage(m_homePage->text().trimmed());
    m_settings->setAskWhereToSave(m_askWhereToSave->isChecked());
    m_settings->setRestoreSessionOnStart(m_restoreSession->isChecked());
    m_settings->setScrollMode(static_cast<Settings::ScrollMode>(m_scrollMode->currentData().toInt()));

    m_settings->setBlockThirdPartyCookies(m_blockThirdPartyCookies->isChecked());
    m_settings->setKeepCookiesOnExit(m_keepCookies->isChecked());
    m_settings->setBlockAds(m_blockAds->isChecked());
    m_settings->setSendDoNotTrack(m_sendDnt->isChecked());
    m_settings->setNotificationsEnabled(m_notifications->isChecked());
    m_settings->setWebRtcPolicy(static_cast<Settings::WebRtcPolicy>(m_webrtcPolicy->currentIndex()));
}

void SettingsDialog::chooseDownloadDirectory()
{
    const QString dir =
        QFileDialog::getExistingDirectory(this, tr("Choose download folder"),
                                          m_downloadDir->text());
    if (!dir.isEmpty()) {
        m_downloadDir->setText(dir);
        if (m_settings) {
            m_settings->setDownloadDirectory(dir);
        }
    }
}

void SettingsDialog::updateCustomEngineEnabled()
{
    const bool custom =
        m_searchEngine->currentData().toString() == QLatin1String(SearchEngines::kCustomId);
    m_customSearchName->setEnabled(custom);
    m_customSearchUrl->setEnabled(custom);
    m_customSearchNote->setVisible(custom);
}

void SettingsDialog::refreshPermissions()
{
    if (!m_permissionList) {
        return;
    }
    m_permissionList->clear();
    if (!m_profile) {
        return;
    }
    for (const auto& entry : m_profile->storedPermissions()) {
        auto* item = new QListWidgetItem(
            tr("%1 - %2: %3").arg(entry.origin, entry.type, entry.state), m_permissionList);
        item->setData(Qt::UserRole, entry.origin);
        item->setData(Qt::UserRole + 1, entry.typeId);
    }
    if (m_permissionList->count() == 0) {
        auto* item = new QListWidgetItem(tr("No site permissions are stored."), m_permissionList);
        item->setFlags(Qt::NoItemFlags);
    }
}

void SettingsDialog::clearAllPermissions()
{
    if (m_profile) {
        m_profile->clearPermissions();
    }
    refreshPermissions();
}

}  // namespace yozora