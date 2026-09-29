// SPDX-License-Identifier: MIT
#pragma once

#include <QDialog>
#include <QString>
#include <QUrl>

class QLabel;
class QPushButton;

namespace yozora {

// The prompt shown when a site asks for a sensitive capability (camera,
// microphone, location, notifications, ...).
//
// It is intentionally a small modal dialog, not a rendered web page: nothing
// the site controls can influence its appearance, and the site cannot react to
// the prompt being open. The safe answer (Block) is the default.
class PermissionPrompt : public QDialog {
    Q_OBJECT

public:
    explicit PermissionPrompt(QWidget* parent = nullptr);

    // `feature` is a verb phrase such as "use your camera". `persistent` tells
    // the user whether keeping the decision is even possible, which is a real
    // property of the requested permission under Chromium.
    void setRequest(const QUrl& origin, const QString& feature, bool persistent);

private:
    QLabel* m_heading = nullptr;
    QLabel* m_detail = nullptr;
    QLabel* m_note = nullptr;
    QPushButton* m_allowButton = nullptr;
    QPushButton* m_blockButton = nullptr;
};

}  // namespace yozora
