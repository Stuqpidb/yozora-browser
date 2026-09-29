// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;
class QWidget;

namespace yozora {

class Settings;

// Minimal settings dialog: search engine, home page, download location and
// theme. Every control writes straight to Settings, which is the single source
// of truth, so this class stays a thin view.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(Settings* settings, QWidget* parent = nullptr);

private:
    QWidget* buildSearchSection();
    QWidget* buildStartupSection();
    QWidget* buildDownloadsSection();
    QWidget* buildAppearanceSection();
    QWidget* buildDataSection();
    QWidget* buildAboutSection();

    void loadFromSettings();
    void applyToSettings();
    void chooseDownloadDirectory();

    Settings* m_settings = nullptr;
    // True while loadFromSettings() fills the widgets. Without it, the
    // programmatic setChecked()/setCurrentIndex() calls look like user edits
    // and write half-loaded values back into Settings.
    bool m_loading = false;

    QComboBox* m_searchEngine = nullptr;
    QLineEdit* m_homePage = nullptr;
    QLineEdit* m_downloadDir = nullptr;
    QPushButton* m_browseButton = nullptr;
    QCheckBox* m_askWhereToSave = nullptr;
    QRadioButton* m_darkTheme = nullptr;
    QRadioButton* m_lightTheme = nullptr;
    QLabel* m_storagePath = nullptr;
    QLabel* m_versionLabel = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
    QCheckBox* m_restoreSession = nullptr;
};

}  // namespace yozora
