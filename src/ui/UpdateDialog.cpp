// SPDX-License-Identifier: MIT
#include "ui/UpdateDialog.h"

#include "core/UpdateChecker.h"
#include "core/Theme.h"

#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace yozora {

UpdateDialog::UpdateDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Yozora Update"));
    setMinimumWidth(460);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 22, 24, 20);
    root->setSpacing(12);

    m_heading = new QLabel(this);
    QFont headingFont = m_heading->font();
    headingFont.setPointSizeF(14.0);
    headingFont.setWeight(QFont::DemiBold);
    m_heading->setFont(headingFont);
    m_heading->setWordWrap(true);
    root->addWidget(m_heading);

    m_notes = new QLabel(this);
    m_notes->setObjectName(QStringLiteral("hintLabel"));
    m_notes->setWordWrap(true);
    m_notes->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_notes->setVisible(false);
    root->addWidget(m_notes);

    m_progress = new QProgressBar(this);
    m_progress->setTextVisible(true);
    m_progress->setVisible(false);
    root->addWidget(m_progress);

    auto* buttons = new QHBoxLayout;
    buttons->setSpacing(8);

    m_runButton = new QPushButton(tr("Run the installer"), this);
    m_runButton->setCursor(Qt::PointingHandCursor);
    m_runButton->setVisible(false);
    connect(m_runButton, &QPushButton::clicked, this, [this] {
        emit runInstallerRequested(m_path);
    });
    buttons->addWidget(m_runButton);

    buttons->addStretch(1);

    m_pageButton = new QPushButton(tr("Open the release page"), this);
    m_pageButton->setObjectName(QStringLiteral("quietButton"));
    m_pageButton->setCursor(Qt::PointingHandCursor);
    m_pageButton->setVisible(false);
    connect(m_pageButton, &QPushButton::clicked, this, [this] {
        emit openPageRequested(m_pageUrl);
    });
    buttons->addWidget(m_pageButton);

    m_downloadButton = new QPushButton(tr("Download"), this);
    m_downloadButton->setCursor(Qt::PointingHandCursor);
    connect(m_downloadButton, &QPushButton::clicked, this, [this] {
        emit downloadRequested();
    });
    buttons->addWidget(m_downloadButton);

    m_closeButton = new QPushButton(tr("Close"), this);
    m_closeButton->setObjectName(QStringLiteral("quietButton"));
    m_closeButton->setCursor(Qt::PointingHandCursor);
    connect(m_closeButton, &QPushButton::clicked, this, [this] {
        emit cancelRequested();
        reject();
    });
    buttons->addWidget(m_closeButton);

    root->addLayout(buttons);
    showState(State::Offer);
}

void UpdateDialog::showState(State state)
{
    const bool downloading = state == State::Downloading;
    const bool downloaded = state == State::Downloaded;

    m_downloadButton->setVisible(state == State::Offer);
    m_downloadButton->setEnabled(state == State::Offer);
    m_progress->setVisible(downloading || downloaded);
    m_runButton->setVisible(downloaded);
    m_closeButton->setText(downloading ? tr("Cancel") : tr("Close"));

    // While the download runs the only sensible action is to wait or stop, so
    // the alternative routes out of the window are hidden.
    m_pageButton->setVisible(state == State::Offer || state == State::Failed);
}

void UpdateDialog::offerRelease(const ReleaseInfo& info)
{
    m_pageUrl = info.pageUrl.isValid() ? info.pageUrl : QUrl(QStringLiteral("about:blank"));
    m_heading->setText(tr("Yozora %1 is available.").arg(info.version.toString()));
    m_notes->setText(info.notes);
    m_notes->setVisible(!info.notes.isEmpty());
    m_progress->setVisible(false);
    showState(State::Offer);

    if (info.assetUrl.isEmpty()) {
        // Newer, but nothing to download: only the release page is on offer.
        m_heading->setText(tr("Yozora %1 is available, but this release has no "
                              "Windows installer.")
                               .arg(info.version.toString()));
        m_notes->setVisible(false);
        m_downloadButton->setVisible(false);
        m_pageButton->setVisible(true);
    }
}

void UpdateDialog::showProblem(const QString& reason)
{
    m_heading->setText(tr("Could not check for updates"));
    m_notes->setText(reason);
    m_notes->setVisible(true);
    m_progress->setVisible(false);
    showState(State::Failed);
    m_downloadButton->setVisible(false);
}

void UpdateDialog::beginDownload(const QString& fileName, qint64 totalBytes)
{
    m_heading->setText(tr("Downloading %1...").arg(fileName));
    m_notes->setVisible(false);
    m_progress->setRange(0, totalBytes > 0 ? 1000 : 0);
    m_progress->setValue(0);
    showState(State::Downloading);
}

void UpdateDialog::setProgress(qint64 received, qint64 totalBytes)
{
    if (totalBytes > 0) {
        m_progress->setRange(0, 1000);
        m_progress->setValue(static_cast<int>(received * 1000 / totalBytes));
        m_progress->setFormat(tr("%p% - %1 of %2")
                                  .arg(QLocale().formattedDataSize(received))
                                  .arg(QLocale().formattedDataSize(totalBytes)));
    } else {
        // No length from the server: a busy indicator rather than a lie about
        // the percentage.
        m_progress->setRange(0, 0);
    }
}

void UpdateDialog::finishDownload(const QString& path, bool sizeMatched)
{
    m_path = path;
    m_heading->setText(tr("The update has been downloaded."));
    m_notes->setText(path);
    m_notes->setVisible(true);
    m_progress->setRange(0, 1000);
    m_progress->setValue(1000);
    showState(State::Downloaded);

    if (!sizeMatched) {
        m_heading->setText(tr("The download did not match the published size."));
    }
}

void UpdateDialog::failDownload(const QString& reason)
{
    m_heading->setText(tr("The update could not be downloaded"));
    m_notes->setText(reason);
    m_notes->setVisible(true);
    m_progress->setVisible(false);
    showState(State::Failed);
}

}  // namespace yozora
