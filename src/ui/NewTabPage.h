// SPDX-License-Identifier: MIT
#pragma once

#include <QString>
#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace yozora {

// Yozora's own start page, drawn with native widgets so it appears instantly
// and needs no bridge into the renderer.
//
// Deliberately minimal: a wordmark, a search field, and nothing else. No news,
// no ads, no accounts, no telemetry, no recommendations.
class NewTabPage : public QWidget {
    Q_OBJECT

public:
    explicit NewTabPage(QWidget* parent = nullptr);

    // Reflects the configured search engine in the placeholder and the button.
    void setSearchEngineName(const QString& name);
    void setSearchEnginePlaceholder(const QString& text);

    // Applies the Yozora palette. `dark` selects the night-sky background.
    void setDarkMode(bool dark);

    // Puts the caret in the search field.
    void focusSearch();

    // Clears any previous query.
    void reset();

signals:
    // The user submitted `query` in the search field.
    void searchRequested(const QString& query);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void submit();
    void restyle();

    QLabel* m_wordmark = nullptr;
    QLabel* m_tagline = nullptr;
    QLineEdit* m_searchField = nullptr;
    QPushButton* m_searchButton = nullptr;
    QLabel* m_hint = nullptr;
    QLabel* m_version = nullptr;

    bool m_dark = true;
    QString m_engineName;
};

}  // namespace yozora
