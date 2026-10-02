// SPDX-License-Identifier: MIT
#pragma once

#include <QList>
#include <QString>
#include <QUrl>
#include <QWidget>

class QGridLayout;
class QLabel;
class QLineEdit;
class QToolButton;
class QVBoxLayout;

namespace yozora {

class GlassField;

// The pinned sites on the start page: a row of site icons standing straight on
// the night sky, with no card around them. Right-click a site to remove it, and
// use the last tile to add one. The list is saved to the user's own state
// folder, never uploaded.
class PinnedSites : public QWidget {
    Q_OBJECT

public:
    explicit PinnedSites(QWidget* parent = nullptr);

    [[nodiscard]] QSize sizeHint() const override;

    // The width the row is laid out in. Tiles are sized to divide it exactly,
    // so the row starts and ends on the same vertical lines as whatever it is
    // meant to line up with - the search field above it. Without this the row is
    // only as wide as it happens to need, and a label wider than its tile
    // pushes the optical centre to the left of the real one.
    void setContentWidth(int width);

    void load();
    void save() const;

signals:
    void openUrl(const QUrl& url);

private:
    struct Site {
        QString url;
        QString title;
    };

    void rebuild();
    void addSite();
    void removeSite(const QString& url);

    QGridLayout* m_grid = nullptr;
    QList<Site> m_sites;
    int m_contentWidth = 0;
};

// The Yozora start page: a wordmark, a search field and the pinned sites, laid
// out in the middle of the night sky and nothing else.
//
// There used to be a board of movable widgets here. It was replaced by this
// fixed layout because a start page that can be rearranged is a page nobody can
// recognise, and because the pinned sites read better directly on the artwork
// than inside a card.
class HomePage : public QWidget {
    Q_OBJECT

public:
    explicit HomePage(QWidget* parent = nullptr);

    void focusSearch();

signals:
    void openUrl(const QUrl& url);
    void searchRequested(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void buildLayout();
    [[nodiscard]] int topSpacing() const;

    QVBoxLayout* m_root = nullptr;
    QLabel* m_logo = nullptr;
    QLabel* m_wordmark = nullptr;
    QLabel* m_tagline = nullptr;
    GlassField* m_field = nullptr;
    QLabel* m_pinsTitle = nullptr;
    PinnedSites* m_pins = nullptr;
};

}  // namespace yozora
