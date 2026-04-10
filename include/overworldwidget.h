#pragma once

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QTimer>
#include <QSet>
#include <QPixmap>
#include <QKeyEvent>

// ═════════════════════════════════════════════════════════════════════════════
//  SpriteSheet
//
//  Physical layout:  8 columns × 9 rows, each cell 68 × 68 px  (544 × 612 total)
//
//  Row 0        — Idle stances:  one static frame per column (8 directions)
//  Rows 1 – 8  — Walk cycles:   6 animated frames (cols 0-5); cols 6-7 empty
// ═════════════════════════════════════════════════════════════════════════════
struct SpriteSheet
{
    QPixmap pixmap;

    static constexpr int FRAME_W = 68;
    static constexpr int FRAME_H = 68;

    QPixmap frame(int col, int row) const
    {
        return pixmap.copy(col * FRAME_W, row * FRAME_H, FRAME_W, FRAME_H);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Direction enum
//
//  Each enumerator value N encodes:
//    idle frame  → row 0, column N
//    walk cycle  → row N+1, columns 0-5
//
//  Sheet rows (0-indexed):
//    Row 0 → Idle (one frame per column, 8 directions)
//    Row 1 → Right
//    Row 2 → Up
//    Row 3 → Forward-right (up-right diagonal)
//    Row 4 → Forward-left  (up-left diagonal)
//    Row 5 → Down
//    Row 6 → Down-right
//    Row 7 → Down-left
//    Row 8 → Left
// ─────────────────────────────────────────────────────────────────────────────
enum class Direction : int
{
    Right        = 0,   // walk row 1
    Up           = 1,   // walk row 2
    ForwardRight = 2,   // walk row 3
    ForwardLeft  = 3,   // walk row 4
    Down         = 4,   // walk row 5
    DownRight    = 5,   // walk row 6
    DownLeft     = 6,   // walk row 7
    Left         = 7    // walk row 8
};

// Helper: walk row for a direction
inline int walkRow(Direction d) { return static_cast<int>(d) + 1; }
// Helper: idle column for a direction
inline int idleCol(Direction d) { return static_cast<int>(d); }

// ─────────────────────────────────────────────────────────────────────────────
//  WASD → Direction mapping
//
//   D        → Right
//   A        → Left
//   W        → Up
//   S        → Down
//   W+D      → ForwardRight
//   W+A      → ForwardLeft
//   S+D      → DownRight
//   S+A      → DownLeft
// ─────────────────────────────────────────────────────────────────────────────

// ─────────────────────────────────────────────────────────────────────────────
//  PlayerSprite
// ─────────────────────────────────────────────────────────────────────────────
class PlayerSprite : public QGraphicsPixmapItem
{
public:
    static constexpr qreal W     = 96.0;   // logical scene size (scaled from 68px)
    static constexpr qreal H     = 96.0;
    static constexpr qreal SPEED = 3.0;

    explicit PlayerSprite(const SpriteSheet &sheet,
                          QGraphicsItem     *parent = nullptr);

    // Call once per physics tick from OverworldWidget::onTick()
    void step(const QSet<int> &heldKeys, const QRectF &worldBounds);

private:
    void setWalkAnim(Direction dir);
    void setIdleFrame(Direction dir);
    void applyFrame();

    const SpriteSheet &m_sheet;

    bool      m_isIdle     = true;
    Direction m_facing     = Direction::Down;   // start facing toward camera
    int       m_frameIndex = 0;
    int       m_tickAccum  = 0;

    static constexpr int TICKS_PER_FRAME = 5;   // animation speed (~12 fps @ 60)
    static constexpr int WALK_FRAMES     = 6;   // frames per walk row (cols 0-5)
};

// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════
class OverworldWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverworldWidget(QWidget *parent = nullptr);

    void activate();
    void deactivate();

signals:
    void dungeonEntered();
    void backToMenu();

protected:
    void keyPressEvent(QKeyEvent   *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void resizeEvent(QResizeEvent  *e) override;

private slots:
    void onTick();

private:
    void buildScene();
    void placePlayer();
    void checkTriggers();
    void fitView();

    static constexpr qreal WORLD_W = 800;
    static constexpr qreal WORLD_H = 600;

    QGraphicsScene    *m_scene       = nullptr;
    QGraphicsView     *m_view        = nullptr;
    PlayerSprite      *m_player      = nullptr;
    QGraphicsRectItem *m_dungeonZone = nullptr;

    QTimer    m_ticker;
    QSet<int> m_heldKeys;

    SpriteSheet m_sheet;
};
