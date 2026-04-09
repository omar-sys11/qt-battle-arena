#include "overworldwidget.h"

#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>

// ═════════════════════════════════════════════════════════════════════════════
//  PlayerSprite
// ═════════════════════════════════════════════════════════════════════════════

PlayerSprite::PlayerSprite(QGraphicsItem *parent)
    : QGraphicsRectItem(0, 0, W, H, parent)
{
    setBrush(QColor("#4a90d9"));
    setPen(QPen(QColor("#1a5a99"), 2));
    // Centre the item's origin so setPos() moves the centre of the sprite.
    setTransformOriginPoint(W / 2.0, H / 2.0);
}

void PlayerSprite::step(const QSet<int> &heldKeys, const QRectF &worldBounds)
{
    qreal dx = 0, dy = 0;

    if (heldKeys.contains(Qt::Key_W) || heldKeys.contains(Qt::Key_Up))    dy -= SPEED;
    if (heldKeys.contains(Qt::Key_S) || heldKeys.contains(Qt::Key_Down))  dy += SPEED;
    if (heldKeys.contains(Qt::Key_A) || heldKeys.contains(Qt::Key_Left))  dx -= SPEED;
    if (heldKeys.contains(Qt::Key_D) || heldKeys.contains(Qt::Key_Right)) dx += SPEED;

    // Normalise diagonal movement so speed stays consistent.
    if (dx != 0 && dy != 0) { dx *= 0.7071; dy *= 0.7071; }

    qreal nx = qBound(worldBounds.left(),           x() + dx, worldBounds.right()  - W);
    qreal ny = qBound(worldBounds.top(),             y() + dy, worldBounds.bottom() - H);
    setPos(nx, ny);
}


// ═════════════════════════════════════════════════════════════════════════════
//  OverworldWidget
// ═════════════════════════════════════════════════════════════════════════════

OverworldWidget::OverworldWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, WORLD_W, WORLD_H, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);  // keys handled at widget level
    layout->addWidget(m_view);

    m_ticker.setInterval(16);  // ~60 fps
    connect(&m_ticker, &QTimer::timeout, this, &OverworldWidget::onTick);

    buildScene();
    setFocusPolicy(Qt::StrongFocus);
    m_view->setTransform(QTransform());  // reset any scaling
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
    // ── Background (grass) ───────────────────────────────────────────────────
    m_scene->setBackgroundBrush(QColor("#4a7c40"));

    // Central dirt path running top-to-bottom
    auto *path = m_scene->addRect(WORLD_W * 0.38, 0,
                                  WORLD_W * 0.24, WORLD_H,
                                  Qt::NoPen, QBrush(QColor("#8d6e3a")));
    path->setZValue(0);

    // ── A simple building (inn / safe house) ─────────────────────────────────
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

    // ── Decorative trees ─────────────────────────────────────────────────────
    const QList<QPointF> trees = {
        {60,  60},  {150, 50},  {680, 55},  {740, 130},
        {60,  430}, {130, 510}, {690, 420}, {750, 300},
        {550, 480}, {580, 80},
    };
    for (const QPointF &p : trees) {
        // Trunk
        m_scene->addRect(p.x() + 8, p.y() + 26, 12, 16,
                         Qt::NoPen, QBrush(QColor("#5d4037")))->setZValue(1);
        // Canopy
        m_scene->addEllipse(p.x(), p.y(), 28, 32,
                            Qt::NoPen, QBrush(QColor("#2e7d32")))->setZValue(2);
    }

    // ── Dungeon entrance (top-centre of map) ─────────────────────────────────
    const qreal dw = 80, dh = 56;
    const qreal dx = (WORLD_W - dw) / 2.0;
    const qreal dy = 6;

    // Stone frame
    m_scene->addRect(dx - 10, dy - 6, dw + 20, dh + 10,
                     Qt::NoPen, QBrush(QColor("#424242")))->setZValue(1);

    // Dark portal (this is also the trigger zone stored in m_dungeonZone)
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

    // ── Player (always on top) ───────────────────────────────────────────────
    m_player = new PlayerSprite();
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();
}

void OverworldWidget::placePlayer()
{
    // Spawn in the centre of the map
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
    // Dungeon door: player bounding rect overlaps the zone rect
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
//  Resize: keep the view fitted to the widget
// ─────────────────────────────────────────────────────────────────────────────

void OverworldWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    fitView();
}

void OverworldWidget::fitView()
{
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
}
