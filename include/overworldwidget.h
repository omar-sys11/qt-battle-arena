#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QTimer>
#include <QKeyEvent>
#include <QSet>

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerSprite  –  a simple coloured rectangle representing the player.
//  Swap for a QGraphicsPixmapItem when you have real art assets.
// ─────────────────────────────────────────────────────────────────────────────
class PlayerSprite : public QGraphicsRectItem
{
public:
    explicit PlayerSprite(QGraphicsItem *parent = nullptr);

    static constexpr qreal W     = 28;
    static constexpr qreal H     = 36;
    static constexpr int   SPEED = 3;   // pixels per frame

    // Advance one frame using currently-held keys, clamped to worldBounds.
    void step(const QSet<int> &heldKeys, const QRectF &worldBounds);
};

// ─────────────────────────────────────────────────────────────────────────────
//  OverworldWidget  –  the safe-zone top-down exploration screen.
//
//  Signals
//  ───────
//    dungeonEntered()   player walked into the dungeon entrance
//    backToMenu()       player pressed Escape
// ─────────────────────────────────────────────────────────────────────────────
class OverworldWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OverworldWidget(QWidget *parent = nullptr);
    ~OverworldWidget() override = default;

    // Call every time this screen becomes visible.
    void activate();
    // Call when navigating away so the timer stops in the background.
    void deactivate();

signals:
    void dungeonEntered();
    void backToMenu();

protected:
    void keyPressEvent  (QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent   *e) override;
    void resizeEvent    (QResizeEvent *e) override;

private slots:
    void onTick();

private:
    // ── Scene objects ────────────────────────────────────────────────────────
    QGraphicsScene    *m_scene      = nullptr;
    QGraphicsView     *m_view       = nullptr;
    PlayerSprite      *m_player     = nullptr;
    QGraphicsRectItem *m_dungeonZone = nullptr;  // overlap → dungeonEntered()

    // ── Game loop ────────────────────────────────────────────────────────────
    QTimer    m_ticker;
    QSet<int> m_heldKeys;

    // ── World dimensions ─────────────────────────────────────────────────────
    static constexpr int WORLD_W = 800;
    static constexpr int WORLD_H = 600;

    // ── Helpers ──────────────────────────────────────────────────────────────
    void buildScene();
    void placePlayer();
    void checkTriggers();
    void fitView();
};
