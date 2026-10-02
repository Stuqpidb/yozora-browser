// SPDX-License-Identifier: MIT
#include "ui/ShieldDialog.h"

#include "core/Theme.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace yozora {

ShieldDialog::ShieldDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Yozora Shield"));
    setMinimumWidth(400);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 18);
    root->setSpacing(10);

    m_summary = new QLabel(this);
    m_summary->setObjectName(QStringLiteral("dialogTitle"));
    m_summary->setWordWrap(true);
    root->addWidget(m_summary);

    m_detail = new QLabel(this);
    m_detail->setObjectName(QStringLiteral("hintLabel"));
    m_detail->setWordWrap(true);
    root->addWidget(m_detail);

    auto* toggleBox = new QWidget(this);
    auto* toggleLayout = new QHBoxLayout(toggleBox);
    toggleLayout->setContentsMargins(0, 6, 0, 0);
    m_siteToggle = new QCheckBox(tr("Block ads and trackers on this site"), toggleBox);
    toggleLayout->addWidget(m_siteToggle);
    toggleLayout->addStretch(1);
    root->addWidget(toggleBox);

    root->addWidget(new QLabel(tr("The bundled lists block known ad and tracker "
                                  "requests. Nothing is fetched from a Yozora server."),
                               this));

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::accept);
    root->addWidget(buttons);

    connect(m_siteToggle, &QCheckBox::toggled, this, [this](bool checked) {
        if (m_updating) {
            return;
        }
        // The checkbox reads "block on this site", the signal reports the
        // inverse: allowing the site means not blocking it.
        emit siteAllowedChanged(!checked);
    });
}

void ShieldDialog::setSite(const QString& host, int blocked, bool enabled, bool allowed)
{
    m_host = host;
    m_updating = true;

    if (!enabled) {
        m_summary->setText(tr("Protection is off"));
        m_detail->setText(tr("Ad and tracker blocking is turned off for all sites in "
                             "Settings."));
        m_siteToggle->setEnabled(false);
    } else if (host.isEmpty()) {
        m_summary->setText(tr("No site in view"));
        m_detail->setText(tr("Open a website to see what the shield blocks."));
        m_siteToggle->setEnabled(false);
    } else {
        m_summary->setText(blocked > 0
                               ? tr("%n item(s) blocked on this page", "", blocked)
                               : tr("Nothing blocked on this page yet"));
        m_detail->setText(tr("Site: %1").arg(host));
        m_siteToggle->setEnabled(true);
        m_siteToggle->setChecked(!allowed);
    }

    m_updating = false;
}

}  // namespace yozora
