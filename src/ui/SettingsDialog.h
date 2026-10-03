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
class QShowEvent;
class QStackedWidget;
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

    // Called by the window once an update check has answered. Kept here so the
    // button and its result text stay in one place.
    void setUpdateCheckResult(const QString& text, bool failed);

signals:
    // The user pressed "Check for updates". The dialog never fetches anything
    // itself; the window owns the UpdateChecker.
    void updateCheckRequested();

protected:
    void showEvent(QShowEvent* event) override;

private:
    struct Section;

    // Wraps a section's page in a header (title + one-line explanation) and a
    // scroll area, so a page with more content than fits does not get clipped and
    // every section starts with the same two lines.
    [[nodiscard]] QWidget* wrapSection(const Section& section, QWidget* parent);
    // Fades the incoming page in and the outgoing one out.
    void showSection(QStackedWidget* stack, int row);

    QWidget* buildSearchSection();
    QWidget* buildStartupSection();
    QWidget* buildDownloadsSection();
    QWidget* buildPrivacySection();
    QWidget* buildInterfaceSection();
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
    QCheckBox* m_blockAds = nullptr;
    QCheckBox* m_sendDnt = nullptr;
    QCheckBox* m_notifications = nullptr;
    QComboBox* m_webrtcPolicy = nullptr;
    QComboBox* m_scrollMode = nullptr;
    QCheckBox* m_sideBarEnabled = nullptr;

    QLabel* m_storagePath = nullptr;
    QListWidget* m_permissionList = nullptr;
    QListWidget* m_nav = nullptr;
    QStackedWidget* m_pages = nullptr;

    QPushButton* m_checkUpdates = nullptr;
    QCheckBox* m_backgroundUpdates = nullptr;
    QCheckBox* m_hardwareAcceleration = nullptr;
    QLabel* m_updateStatus = nullptr;
    QLabel* m_versionLabel = nullptr;
    QDialogButtonBox* m_buttons = nullptr;
};

}  // namespace yozora