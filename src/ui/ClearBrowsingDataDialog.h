// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;

namespace yozora {

class WebProfile;

// "Clear browsing data", the honest version.
//
// It only offers what the engine can actually do right now. Cookies, the HTTP
// cache, visited-link state and site permissions are removed immediately;
// per-site storage that Chromium keeps open while it runs (localStorage,
// IndexedDB, service workers) is queued for the next start instead of pretending
// to have been removed.
class ClearBrowsingDataDialog : public QDialog {
    Q_OBJECT

public:
    explicit ClearBrowsingDataDialog(WebProfile* profile, QWidget* parent = nullptr);

private:
    void clearNow();

    WebProfile* m_profile = nullptr;
    QComboBox* m_range = nullptr;
    QCheckBox* m_cookies = nullptr;
    QCheckBox* m_cache = nullptr;
    QCheckBox* m_visited = nullptr;
    QCheckBox* m_permissions = nullptr;
    QLabel* m_status = nullptr;
};

}  // namespace yozora
