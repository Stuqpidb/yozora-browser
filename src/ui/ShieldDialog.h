// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <QString>

class QCheckBox;
class QLabel;

namespace yozora {

// The shield panel, in the spirit of Brave's.
//
// It reports what was blocked on the current page and lets the user allow the
// site (which turns protection off for that host only). It never fetches
// anything and never edits the filter list; the window owns those decisions and
// this dialog only reports them through its signal.
class ShieldDialog : public QDialog {
    Q_OBJECT

public:
    ShieldDialog(QWidget* parent = nullptr);

    // Fills the panel for a page. `host` is used in the wording, `blocked` is
    // the per-page count, `enabled` is the global switch and `allowed` is
    // whether this host is already on the allowlist.
    void setSite(const QString& host, int blocked, bool enabled, bool allowed);

signals:
    // The user changed the "block on this site" toggle. The window applies it
    // to Settings and reloads the page.
    void siteAllowedChanged(bool allowed);

private:
    QLabel* m_summary = nullptr;
    QLabel* m_detail = nullptr;
    QCheckBox* m_siteToggle = nullptr;
    QString m_host;
    bool m_updating = false;
};

}  // namespace yozora
