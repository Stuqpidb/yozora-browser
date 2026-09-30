// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QWidget;

namespace yozora {

class Settings;
class WebProfile;

// Yozora settings. Every control writes straight to Settings (or to the
// profile for the few operations that are not preferences), which keeps this
// class a thin view and makes the privacy defaults visible in one place.
class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    SettingsDialog(Settings* settings, WebProfile* profile, QWidget* parent = nullptr);

private:
    QWidget* buildSearchSection();
    QWidget* buildStartupSection();
    QWidget* buildDownloadsSection();
    QWidget* buildPrivacySection();
    QWidget* buildScrollingSection();
    QWidget* buildDataSection();
    QWidget* buildAboutSection();

    void loadFromSettings();
    void applyToSettings();
    void chooseDownloadDirectory();
    void updateCustomEngineEnabled();
    void refreshPermissions();
    void clearAllPermissions();

    Settings* m_settings = nullptr;
    WebProfile* m_profile = nullptr;
    // True while loadFromSettings() fills the widgets. Without it, the
    // programmatic setChecked()/setCurrentIndex() calls look like user edits
    // and write half-loaded values back into Settings.
    bool m_loading = false;

    QComboBox* m_searchEngine = nullptr;
    QLineEdit* m_customSearchName = nullptr;
    QLineEdit* m_customSearchUrl = nullptr;
    QLabel* m_customSearchNote = nullptr;

    QLineEdit* m_homePage = nullptr;
    QCheckBox* m_restoreSession = nullptr;

    QLineEdit* m_downloadDir = nullptr;
    QPushButton* m_browseButton = nullptr;
    QCheckBox* m_askWhereToSave = nullptr;

    QCheckBox* m_blockThirdPartyCookies = nullptr;
    QCheckBox* m_keepCookies = nullptr;
    QCheckBox* m_blockTrackers = nullptr;
    QCheckBox* m_sendDnt = nullptr;
    QCheckBox* m_notifications = nullptr;
    QComboBox* m_webrtcPolicy = nullptr;
    QComboBox* m_scrollMode = nullptr;

    QLabel* m_storagePath = nullptr;
    QListWidget* m_permissionList = nullptr;
    QListWidget* m_nav = nullptr;

    QLabel* m_versionLabel = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};

}  // namespace yozora
