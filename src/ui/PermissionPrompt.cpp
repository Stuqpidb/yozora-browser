// SPDX-License-Identifier: MIT
#include "ui/PermissionPrompt.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace yozora {

PermissionPrompt::PermissionPrompt(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Permission request"));
    setModal(true);
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(24, 22, 24, 20);
    layout->setSpacing(10);

    m_heading = new QLabel(this);
    m_heading->setWordWrap(true);
    m_heading->setStyleSheet(QStringLiteral("font-size: 15px; font-weight: 600;"));
    layout->addWidget(m_heading);

    m_detail = new QLabel(this);
    m_detail->setObjectName(QStringLiteral("hintLabel"));
    m_detail->setWordWrap(true);
    layout->addWidget(m_detail);

    m_note = new QLabel(this);
    m_note->setObjectName(QStringLiteral("hintLabel"));
    m_note->setWordWrap(true);
    layout->addWidget(m_note);

    layout->addSpacing(6);

    auto* buttons = new QHBoxLayout;
    buttons->addStretch(1);

    m_blockButton = new QPushButton(tr("Block"), this);
    m_blockButton->setDefault(true);  // the safe answer is the default
    connect(m_blockButton, &QPushButton::clicked, this, &QDialog::reject);
    buttons->addWidget(m_blockButton);

    m_allowButton = new QPushButton(tr("Allow"), this);
    connect(m_allowButton, &QPushButton::clicked, this, &QDialog::accept);
    buttons->addWidget(m_allowButton);

    layout->addLayout(buttons);
}

void PermissionPrompt::setRequest(const QUrl& origin, const QString& feature, bool persistent)
{
    const QString site = origin.host().isEmpty() ? origin.toString() : origin.host();
    m_heading->setText(tr("%1 wants to %2").arg(site, feature));
    m_detail->setText(origin.toString());
    m_note->setText(persistent ? tr("Your choice will be remembered for this site. You can "
                                    "change it later in Settings > Privacy.")
                               : tr("Yozora will ask again the next time this site requests it."));
}

}  // namespace yozora
