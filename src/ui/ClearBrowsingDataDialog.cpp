// SPDX-License-Identifier: MIT
#include "ui/ClearBrowsingDataDialog.h"

#include "web/WebProfile.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QStandardItemModel>
#include <QVBoxLayout>

namespace yozora {

ClearBrowsingDataDialog::ClearBrowsingDataDialog(WebProfile* profile, QWidget* parent)
    : QDialog(parent)
    , m_profile(profile)
{
    setWindowTitle(tr("Clear browsing data"));
    setMinimumWidth(460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 22, 18);
    root->setSpacing(12);

    auto* rangeBox = new QGroupBox(tr("Time range"), this);
    auto* rangeLayout = new QFormLayout(rangeBox);
    m_range = new QComboBox(rangeBox);
    m_range->addItem(tr("Last hour"));
    m_range->addItem(tr("Last 24 hours"));
    m_range->addItem(tr("Last 7 days"));
    m_range->addItem(tr("All time"));
    m_range->setCurrentIndex(3);
    // Only "All time" can be honoured today. The engine does not expose
    // timestamps for cookies or cache entries, so the shorter ranges are shown
    // for context but not selectable, rather than silently clearing everything.
    if (auto* model = qobject_cast<QStandardItemModel*>(m_range->model())) {
        for (int i = 0; i < 3 && i < model->rowCount(); ++i) {
            if (auto* item = model->item(i)) {
                item->setEnabled(false);
            }
        }
    }
    rangeLayout->addRow(tr("Delete data from:"), m_range);
    root->addWidget(rangeBox);

    auto* dataBox = new QGroupBox(tr("Data to remove"), this);
    auto* dataLayout = new QVBoxLayout(dataBox);

    m_cookies = new QCheckBox(tr("Cookies and other site data"), dataBox);
    m_cookies->setChecked(true);
    m_cache = new QCheckBox(tr("Cached images and files"), dataBox);
    m_cache->setChecked(true);
    m_visited = new QCheckBox(tr("Visited links"), dataBox);
    m_visited->setChecked(true);
    m_permissions = new QCheckBox(tr("Site permissions (camera, location, ...)"), dataBox);
    m_permissions->setChecked(true);

    dataLayout->addWidget(m_cookies);
    dataLayout->addWidget(m_cache);
    dataLayout->addWidget(m_visited);
    dataLayout->addWidget(m_permissions);
    root->addWidget(dataBox);

    m_status = new QLabel(this);
    m_status->setObjectName(QStringLiteral("hintLabel"));
    m_status->setWordWrap(true);
    root->addWidget(m_status);

    auto* buttons = new QDialogButtonBox(this);
    auto* clearButton = buttons->addButton(tr("Clear"), QDialogButtonBox::AcceptRole);
    clearButton->setDefault(true);
    buttons->addButton(QDialogButtonBox::Close);
    connect(clearButton, &QPushButton::clicked, this, &ClearBrowsingDataDialog::clearNow);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(buttons);
}

void ClearBrowsingDataDialog::clearNow()
{
    if (!m_profile) {
        m_status->setText(tr("There is nothing to clear."));
        return;
    }

    bool queuedRestart = false;

    if (m_cookies->isChecked()) {
        m_profile->clearCookies();
        // localStorage, IndexedDB, service workers and friends stay open while
        // the browser runs; wiping them is queued to the next start.
        WebProfile::requestSiteStoragePurge();
        queuedRestart = true;
    }
    if (m_cache->isChecked()) {
        m_profile->clearCache();
    }
    if (m_visited->isChecked()) {
        m_profile->clearVisitedLinks();
    }
    if (m_permissions->isChecked()) {
        m_profile->clearPermissions();
    }

    m_status->setText(queuedRestart
                          ? tr("Cleared. The remaining site storage will be removed the next "
                               "time Yozora starts.")
                          : tr("Browsing data cleared."));
}

}  // namespace yozora
