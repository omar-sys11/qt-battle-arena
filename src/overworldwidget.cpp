#include "overworldwidget.h"

#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QKeyEvent>
#include <QDir>
#include <QFileInfo>
#include <QDebug>

// ═════════════════════════════════════════════════════════════════════════════
//  PlayerSprite
// ═════════════════════════════════════════════════════════════════════════════

PlayerSprite::PlayerSprite(const SpriteSheet &sheet, QGraphicsItem *parent)
    : QGraphicsPixmapItem(parent)
    , m_sheet(sheet)
{
    setTransformationMode(Qt::FastTransformation);
    setTransformOriginPoint(W / 2.0, H / 2.0);

    // Start idle, facing down (toward camera)
    setIdleFrame(Direction::Down);
}

// ─────────────────────────────────────────────────────────────────────────────

void PlayerSprite::setIdleFrame(Direction dir)
{
    m_isIdle     = true;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void PlayerSprite::setWalkAnim(Direction dir)
{
    // Only reset the frame counter when we actually change direction,
    // so the walk cycle doesn't stutter while holding the same key.
    if (!m_isIdle && m_facing == dir) return;

    m_isIdle     = false;
    m_facing     = dir;
    m_frameIndex = 0;
    m_tickAccum  = 0;
    applyFrame();
}

void PlayerSprite::applyFrame()
{
    int col, row;

    if (m_isIdle) {
        // Row 0, column = direction index
        row = 0;
        col = idleCol(m_facing);
    } else {
        // Walk row = direction index + 1, column = current frame (0-5)
        row = walkRow(m_facing);
        col = m_frameIndex;
    }

    QPixmap raw = m_sheet.frame(col, row);
    setPixmap(raw.scaled(static_cast<int>(W),
                         static_cast<int>(H),
                         Qt::KeepAspectRatio,
                         Qt::FastTransformation));
}

// ─────────────────────────────────────────────────────────────────────────────

void PlayerSprite::step(const QSet<int> &heldKeys, const QRectF &worldBounds)
{
    const bool goUp    = heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up);
    const bool goDown  = heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down);
    const bool goLeft  = heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left);
    const bool goRight = heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right);

    qreal dx = 0, dy = 0;
    if (goUp)    dy -= SPEED;
    if (goDown)  dy += SPEED;
    if (goLeft)  dx -= SPEED;
    if (goRight) dx += SPEED;

    // Normalise diagonal so speed is consistent
    if (dx != 0 && dy != 0) { dx *= 0.7071; dy *= 0.7071; }

    const bool moving = (dx != 0 || dy != 0);

    // ── Choose direction ──────────────────────────────────────────────────
    //
    //  8-way WASD → Direction mapping:
    //
    //   D       → Right
    //   A       → Left
    //   W       → Up
    //   S       → Down
    //   W+D     → ForwardRight
    //   W+A     → ForwardLeft
    //   S+D     → DownRight
    //   S+A     → DownLeft
    //
    if (moving) {
        Direction dir;
        if      (goUp   && goRight) dir = Direction::ForwardRight;
        else if (goUp   && goLeft)  dir = Direction::ForwardLeft;
        else if (goDown && goRight) dir = Direction::DownRight;
        else if (goDown && goLeft)  dir = Direction::DownLeft;
        else if (goRight)           dir = Direction::Right;
        else if (goLeft)            dir = Direction::Left;
        else if (goUp)              dir = Direction::Up;
        else                        dir = Direction::Down;

        setWalkAnim(dir);

        // Advance animation frame every TICKS_PER_FRAME physics ticks
        ++m_tickAccum;
        if (m_tickAccum >= TICKS_PER_FRAME) {
            m_tickAccum  = 0;
            m_frameIndex = (m_frameIndex + 1) % WALK_FRAMES;
            applyFrame();
        }
    } else {
        // Stopped — show idle stance for last known facing direction
        if (!m_isIdle) {
            setIdleFrame(m_facing);
        }
    }

    // ── Clamp to world and move ───────────────────────────────────────────
    qreal nx = qBound(worldBounds.left(),  x() + dx, worldBounds.right()  - W);
    qreal ny = qBound(worldBounds.top(),   y() + dy, worldBounds.bottom() - H);
    setPos(nx, ny);
}


// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════

OverworldWidget::OverworldWidget(QWidget *parent)
    : QWidget(parent)
{
    // ── Load sprite sheet ────────────────────────────────────────────────────
    QString spritePath = "resources/sprites/player.png";
    qDebug() << "Working directory:" << QDir::currentPath();
    qDebug() << "Absolute sprite path:" << QFileInfo(spritePath).absoluteFilePath();
    qDebug() << "File exists on disk:" << QFileInfo(spritePath).exists();

    m_sheet.pixmap = QPixmap(spritePath);

    if (m_sheet.pixmap.isNull()) {
        qFatal("Could not load resources/sprites/player.png");
    }

    // ── Layout ───────────────────────────────────────────────────────────────
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_ticker.setInterval(16);   // ~60 fps
    connect(&m_ticker, &QTimer::timeout, this, &OverworldWidget::onTick);

    buildScene();
    setFocusPolicy(Qt::StrongFocus);
    m_view->setTransform(QTransform());
    m_view->setSceneRect(0, 0, WORLD_W, WORLD_H);
}

// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::activate()
{
    m_heldKeys.clear();
    placePlayer();
    m_ticker.start();
    setFocus();
}

void OverworldWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene construction
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::buildScene()
{
    // ── Background ───────────────────────────────────────────────────────────
    QPixmap grassTile("resources/sprites/grass.png");

    const int TILE_SIZE = 64; // choose 32 or 64 depending on your game
    
    QPixmap scaledTile = grassTile.scaled(
        TILE_SIZE,
        TILE_SIZE,
        Qt::IgnoreAspectRatio,
        Qt::FastTransformation
    );
    
    for (int y = 0; y < WORLD_H; y += TILE_SIZE) {
        for (int x = 0; x < WORLD_W; x += TILE_SIZE) {
            QGraphicsPixmapItem *tile = m_scene->addPixmap(scaledTile);
            tile->setPos(x, y);
            tile->setZValue(0);
        }
    }

    // Central dirt path
    auto *path = m_scene->addRect(WORLD_W * 0.38, 0,
                                  WORLD_W * 0.24, WORLD_H,
                                  Qt::NoPen, QBrush(QColor("#8d6e3a")));
    path->setZValue(0);

    // ── Inn ──────────────────────────────────────────────────────────────────
    auto *walls = m_scene->addRect(80, 220, 140, 110,
                                   QPen(QColor("#4e342e"), 2),
                                   QBrush(QColor("#8d6e63")));
    walls->setZValue(1);

    auto *roof = m_scene->addRect(68, 198, 164, 34,
                                  QPen(QColor("#b71c1c"), 2),
                                  QBrush(QColor("#e53935")));
    roof->setZValue(2);

    auto *innLabel = m_scene->addText("INN", QFont("Arial", 9, QFont::Bold));
    innLabel->setDefaultTextColor(Qt::white);
    innLabel->setPos(134, 258);
    innLabel->setZValue(3);

    // ── Trees ────────────────────────────────────────────────────────────────
    const QList<QPointF> trees = {
        {60,  60},  {150, 50},  {680, 55},  {740, 130},
        {60,  430}, {130, 510}, {690, 420}, {750, 300},
        {550, 480}, {580, 80},
    };
    for (const QPointF &p : trees) {
        m_scene->addRect(p.x() + 8, p.y() + 26, 12, 16,
                         Qt::NoPen, QBrush(QColor("#5d4037")))->setZValue(1);
        m_scene->addEllipse(p.x(), p.y(), 28, 32,
                            Qt::NoPen, QBrush(QColor("#2e7d32")))->setZValue(2);
    }

    // ── Dungeon entrance ─────────────────────────────────────────────────────
    const qreal dw = 80, dh = 56;
    const qreal dx = (WORLD_W - dw) / 2.0;
    const qreal dy = 6;

    m_scene->addRect(dx - 10, dy - 6, dw + 20, dh + 10,
                     Qt::NoPen, QBrush(QColor("#424242")))->setZValue(1);

    m_dungeonZone = m_scene->addRect(dx, dy, dw, dh,
                                     Qt::NoPen, QBrush(QColor("#1a1a2e")));
    m_dungeonZone->setZValue(2);

    auto *skull = m_scene->addText("☠  DUNGEON", QFont("Arial", 8, QFont::Bold));
    skull->setDefaultTextColor(QColor("#ef5350"));
    skull->setPos(dx + 2, dy + dh + 4);
    skull->setZValue(3);

    // ── HUD hint ─────────────────────────────────────────────────────────────
    auto *hint = m_scene->addText(
        "WASD / Arrow keys to move    ESC = menu",
        QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffffaa"));
    hint->setPos(8, WORLD_H - 20);
    hint->setZValue(10);

    // ── Player ───────────────────────────────────────────────────────────────
    m_player = new PlayerSprite(m_sheet);
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();
}

void OverworldWidget::placePlayer()
{
    if (m_player)
        m_player->setPos((WORLD_W - PlayerSprite::W) / 2.0,
                         (WORLD_H - PlayerSprite::H) / 2.0);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::onTick()
{
    m_player->step(m_heldKeys, QRectF(0, 0, WORLD_W, WORLD_H));
    checkTriggers();
}

void OverworldWidget::checkTriggers()
{
    if (m_player->collidesWithItem(m_dungeonZone)) {
        deactivate();
        emit dungeonEntered();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        deactivate();
        emit backToMenu();
        return;
    }
    m_heldKeys.insert(e->key());
}

void OverworldWidget::keyReleaseEvent(QKeyEvent *e)
{
    m_heldKeys.remove(e->key());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resize
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
}

void OverworldWidget::fitView()
{
}
