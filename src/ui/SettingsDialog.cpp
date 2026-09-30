// SPDX-License-Identifier: MIT
#include "ui/SettingsDialog.h"

#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "ui/ClearBrowsingDataDialog.h"
#include "utils/Version.h"
#include "web/WebProfile.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabWidget>
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

}  // namespace

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

    m_nav = new QListWidget(this);
    m_nav->setObjectName(QStringLiteral("settingsNav"));
    m_nav->setFixedWidth(210);
    m_nav->setFocusPolicy(Qt::NoFocus);
    root->addWidget(m_nav);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto* stack = new QStackedWidget(right);
    struct Section {
        QString title;
        QWidget* page;
    };
    const QList<Section> sections = {
        {tr("Search"), buildSearchSection()},
        {tr("Startup"), buildStartupSection()},
        {tr("Downloads"), buildDownloadsSection()},
        {tr("Privacy"), buildPrivacySection()},
        {tr("Scrolling"), buildScrollingSection()},
        {tr("Data"), buildDataSection()},
        {tr("About"), buildAboutSection()},
    };
    for (const Section& section : sections) {
        m_nav->addItem(section.title);
        stack->addWidget(section.page);
    }
    connect(m_nav, &QListWidget::currentRowChanged, stack, &QStackedWidget::setCurrentIndex);
    m_nav->setCurrentRow(0);

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
    connect(m_blockTrackers, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_sendDnt, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_notifications, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_webrtcPolicy, &QComboBox::currentIndexChanged, this, [this](int) {
        applyToSettings();
    });

    loadFromSettings();
    updateCustomEngineEnabled();
    refreshPermissions();
}

QWidget* SettingsDialog::buildSearchSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("Search engine"), page);
    auto* form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignLeft);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("New tab"), page);
    auto* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("Files"), page);
    auto* form = new QFormLayout(box);
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* cookies = new QGroupBox(tr("Cookies"), page);
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

    auto* tracking = new QGroupBox(tr("Tracking protection"), page);
    auto* trackingLayout = new QVBoxLayout(tracking);
    m_blockTrackers = new QCheckBox(tr("Block requests to known tracking domains"), tracking);
    m_sendDnt = new QCheckBox(tr("Send \"Do Not Track\" and \"Global Privacy Control\" signals"),
                              tracking);
    trackingLayout->addWidget(m_blockTrackers);
    trackingLayout->addWidget(m_sendDnt);
    trackingLayout->addWidget(hint(tr("The tracker list ships with Yozora and is applied "
                                      "locally. Nothing is ever fetched from Yozora's servers."),
                                   tracking));
    layout->addWidget(tracking);

    auto* notifications = new QGroupBox(tr("Notifications"), page);
    auto* notificationsLayout = new QVBoxLayout(notifications);
    m_notifications = new QCheckBox(tr("Allow sites to ask to show notifications"), notifications);
    notificationsLayout->addWidget(m_notifications);
    notificationsLayout->addWidget(hint(tr("Sites still have to ask for permission. Turning this "
                                           "off refuses every notification request without "
                                           "asking."),
                                        notifications));
    layout->addWidget(notifications);

    auto* network = new QGroupBox(tr("Network / WebRTC"), page);
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
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("Scrolling"), page);
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
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("Browsing data"), page);
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

    auto* permsBox = new QGroupBox(tr("Stored site permissions"), page);
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
    layout->setContentsMargins(22, 22, 22, 22);
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
    layout->addWidget(separator());
    layout->addStretch(1);
    return page;
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
        m_blockTrackers->setChecked(m_settings->blockTrackers());
        m_sendDnt->setChecked(m_settings->sendDoNotTrack());
        m_notifications->setChecked(m_settings->notificationsEnabled());
        m_webrtcPolicy->setCurrentIndex(static_cast<int>(m_settings->webrtcPolicy()));

        m_scrollMode->setCurrentIndex(
            m_scrollMode->findData(static_cast<int>(m_settings->scrollMode())));
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
    m_settings->setBlockTrackers(m_blockTrackers->isChecked());
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
