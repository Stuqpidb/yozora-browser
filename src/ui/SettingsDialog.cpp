// SPDX-License-Identifier: MIT
#include "ui/SettingsDialog.h"

#include "core/SearchEngine.h"
#include "core/Settings.h"
#include "utils/Version.h"

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
#include <QPushButton>
#include <QRadioButton>
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

}  // namespace

SettingsDialog::SettingsDialog(Settings* settings, QWidget* parent)
    : QDialog(parent)
    , m_settings(settings)
{
    setWindowTitle(tr("Yozora Settings"));
    setMinimumSize(640, 520);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* pages = new QTabWidget(this);
    pages->setDocumentMode(true);
    pages->addTab(buildSearchSection(), tr("Search"));
    pages->addTab(buildStartupSection(), tr("Startup"));
    pages->addTab(buildDownloadsSection(), tr("Downloads"));
    pages->addTab(buildAppearanceSection(), tr("Appearance"));
    pages->addTab(buildDataSection(), tr("Data"));
    pages->addTab(buildAboutSection(), tr("About"));
    root->addWidget(pages);

    m_buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(m_buttons);

    // Settings apply immediately, the way desktop browsers behave.
    connect(m_searchEngine, &QComboBox::currentIndexChanged, this,
            [this](int) { applyToSettings(); });
    connect(m_homePage, &QLineEdit::editingFinished, this, &SettingsDialog::applyToSettings);
    connect(m_askWhereToSave, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_restoreSession, &QCheckBox::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_darkTheme, &QRadioButton::toggled, this, &SettingsDialog::applyToSettings);
    connect(m_lightTheme, &QRadioButton::toggled, this, &SettingsDialog::applyToSettings);

    loadFromSettings();
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
    form->addRow(tr("Default engine:"), m_searchEngine);

    auto* note = new QLabel(
        tr("Used whenever you type something that is not an address.\n"
           "DuckDuckGo is the default because it does not require an account."),
        box);
    note->setObjectName(QStringLiteral("hintLabel"));
    note->setWordWrap(true);
    form->addRow(QString(), note);

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

    auto* note = new QLabel(
        tr("Use about:yozora for the Yozora start page, or any address."), box);
    note->setObjectName(QStringLiteral("hintLabel"));
    note->setWordWrap(true);
    form->addRow(QString(), note);

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
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::buildAppearanceSection()
{
    auto* page = new QWidget(this);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(22, 22, 22, 22);
    layout->setSpacing(12);

    auto* box = new QGroupBox(tr("Theme"), page);
    auto* boxLayout = new QVBoxLayout(box);

    m_darkTheme = new QRadioButton(tr("Dark (night sky)"), box);
    m_lightTheme = new QRadioButton(tr("Light"), box);
    boxLayout->addWidget(m_darkTheme);
    boxLayout->addWidget(m_lightTheme);
    boxLayout->addStretch(1);

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

    auto* clear = new QPushButton(tr("Clear cookies, cache and site data"), box);
    connect(clear, &QPushButton::clicked, this, [this] {
        if (m_settings) {
            m_settings->resetToDefaults();
        }
        m_storagePath->setText(tr("Browsing data cleared. Restart Yozora to start fresh."));
    });
    boxLayout->addWidget(clear);
    boxLayout->addStretch(1);

    layout->addWidget(box);
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

    auto* engine = new QLabel(
        tr("Rendering engine: Qt WebEngine (Chromium)\n"
           "Yozora is an independent project and is not affiliated with Google or Mozilla."),
        page);
    engine->setObjectName(QStringLiteral("hintLabel"));
    engine->setWordWrap(true);
    layout->addWidget(engine);
    layout->addWidget(separator());
    layout->addStretch(1);
    return page;
}

void SettingsDialog::loadFromSettings()
{
    if (!m_settings) {
        return;
    }
    const int index = m_searchEngine->findData(m_settings->searchEngineId());
    m_searchEngine->setCurrentIndex(index >= 0 ? index : 0);

    m_homePage->setText(m_settings->homePage());
    m_downloadDir->setText(m_settings->downloadDirectory());
    m_askWhereToSave->setChecked(m_settings->askWhereToSave());
    m_restoreSession->setChecked(m_settings->restoreSessionOnStart());

    const bool dark = m_settings->themeMode() != Settings::ThemeMode::Light;
    m_darkTheme->setChecked(dark);
    m_lightTheme->setChecked(!dark);
}

void SettingsDialog::applyToSettings()
{
    if (!m_settings) {
        return;
    }
    m_settings->setSearchEngineId(m_searchEngine->currentData().toString());
    m_settings->setHomePage(m_homePage->text().trimmed());
    m_settings->setAskWhereToSave(m_askWhereToSave->isChecked());
    m_settings->setRestoreSessionOnStart(m_restoreSession->isChecked());
    m_settings->setThemeMode(m_darkTheme->isChecked() ? Settings::ThemeMode::Dark
                                                       : Settings::ThemeMode::Light);
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

}  // namespace yozora
