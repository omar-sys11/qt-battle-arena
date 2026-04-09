#include "dungeonwidget.h"

#include <QVBoxLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QRandomGenerator>

// ═════════════════════════════════════════════════════════════════════════════
//  EnemySprite
// ═════════════════════════════════════════════════════════════════════════════

// Colour per enemy type so they're visually distinct
static QColor enemyColour(CharacterType t)
{
    switch (t) {
    case CharacterType::Warrior: return QColor("#c62828");  // red
    case CharacterType::Mage:    return QColor("#6a1b9a");  // purple
    case CharacterType::Archer:  return QColor("#2e7d32");  // dark green
    }
    return Qt::gray;
}

EnemySprite::EnemySprite(CharacterType type, const QString &name, QGraphicsItem *parent)
    : QGraphicsRectItem(0, 0, W, H, parent)
    , m_type(type)
    , m_name(name)
{
    setBrush(enemyColour(type));
    setPen(QPen(Qt::black, 1));

    // Random starting velocity so enemies don't all move identically
    auto *rng = QRandomGenerator::global();
    m_vx = (rng->bounded(2) == 0 ? 1 : -1) * (1 + rng->bounded(2));
    m_vy = (rng->bounded(2) == 0 ? 1 : -1) * (1 + rng->bounded(2));
}

void EnemySprite::patrol(const QRectF &worldBounds)
{
    qreal nx = x() + m_vx;
    qreal ny = y() + m_vy;

    // Bounce off world edges
    if (nx < worldBounds.left()       || nx > worldBounds.right()  - W) { m_vx = -m_vx; nx = x(); }
    if (ny < worldBounds.top() + 60   || ny > worldBounds.bottom() - H) { m_vy = -m_vy; ny = y(); }

    setPos(nx, ny);
}


// ═════════════════════════════════════════════════════════════════════════════
//  DungeonWidget
// ═════════════════════════════════════════════════════════════════════════════

DungeonWidget::DungeonWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_scene = new QGraphicsScene(0, 0, 800, 600, this);

    m_view = new QGraphicsView(m_scene, this);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_view->setRenderHint(QPainter::Antialiasing);
    m_view->setFocusPolicy(Qt::NoFocus);
    layout->addWidget(m_view);

    m_ticker.setInterval(16);  // ~60 fps
    connect(&m_ticker, &QTimer::timeout, this, &DungeonWidget::onTick);

    buildScene();
    setFocusPolicy(Qt::StrongFocus);
    m_view->setTransform(QTransform());
    m_view->setSceneRect(0, 0, 800, 600);
}

// ─────────────────────────────────────────────────────────────────────────────

void DungeonWidget::activate()
{
    m_heldKeys.clear();
    placePlayer();

    // Re-randomise enemy positions each visit so the dungeon feels fresh
    spawnEnemies();

    m_ticker.start();
    setFocus();
}

void DungeonWidget::deactivate()
{
    m_ticker.stop();
    m_heldKeys.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Scene construction
// ─────────────────────────────────────────────────────────────────────────────

void DungeonWidget::buildScene()
{
    // ── Dark stone floor ─────────────────────────────────────────────────────
    m_scene->setBackgroundBrush(QColor("#1a1a2e"));

    // Stone tile grid (drawn as lighter rectangles for a tiled look)
    const int tileW = 80, tileH = 60;
    for (int col = 0; col < WORLD_W / tileW; ++col) {
        for (int row = 0; row < WORLD_H / tileH; ++row) {
            m_scene->addRect(col * tileW + 1, row * tileH + 1,
                             tileW - 2, tileH - 2,
                             QPen(QColor("#263238"), 1),
                             QBrush(QColor("#263238")))->setZValue(0);
        }
    }

    // ── Wall pillars ─────────────────────────────────────────────────────────
    const QList<QPointF> pillars = {
        {160, 120}, {400, 120}, {620, 120},
        {160, 340}, {400, 340}, {620, 340},
        {80,  460}, {500, 460},
    };
    for (const QPointF &p : pillars) {
        m_scene->addRect(p.x(), p.y(), 32, 48,
                         QPen(QColor("#37474f"), 2),
                         QBrush(QColor("#455a64")))->setZValue(1);
    }

    // ── Torches (glowing yellow dots) ────────────────────────────────────────
    const QList<QPointF> torches = {
        {100, 80}, {440, 80}, {720, 80},
        {100, 480},{440, 480},{720, 480},
    };
    for (const QPointF &t : torches) {
        auto *glow = m_scene->addEllipse(t.x() - 8, t.y() - 8, 20, 20,
                                         Qt::NoPen, QBrush(QColor("#ff6f0080")));
        glow->setZValue(1);
        m_scene->addEllipse(t.x() - 2, t.y() - 2, 8, 8,
                            Qt::NoPen, QBrush(QColor("#ffca28")))->setZValue(2);
    }

    // ── Exit portal (bottom-centre – leads back to overworld) ────────────────
    const qreal ew = 80, eh = 56;
    const qreal ex = (WORLD_W - ew) / 2.0;
    const qreal ey = WORLD_H - eh - 6;

    m_scene->addRect(ex - 10, ey - 6, ew + 20, eh + 10,
                     Qt::NoPen, QBrush(QColor("#1b5e20")))->setZValue(1);

    m_exitZone = m_scene->addRect(ex, ey, ew, eh,
                                  Qt::NoPen, QBrush(QColor("#00c853")));
    m_exitZone->setZValue(2);

    auto *exitLabel = m_scene->addText("EXIT ↑", QFont("Arial", 8, QFont::Bold));
    exitLabel->setDefaultTextColor(Qt::white);
    exitLabel->setPos(ex + 12, ey + 18);
    exitLabel->setZValue(3);

    // ── HUD hint ─────────────────────────────────────────────────────────────
    auto *hint = m_scene->addText(
        "Touch a red enemy to fight!    ESC = menu",
        QFont("Arial", 7));
    hint->setDefaultTextColor(QColor("#ffffff88"));
    hint->setPos(8, 4);
    hint->setZValue(10);

    // ── Player (placeholder – overwritten each activate()) ───────────────────
    m_player = new QGraphicsRectItem(0, 0, PW, PH);
    m_player->setBrush(QColor("#4a90d9"));
    m_player->setPen(QPen(QColor("#1a5a99"), 2));
    m_player->setZValue(9);
    m_scene->addItem(m_player);
    placePlayer();

    // Enemies are spawned on activate(), not here.
}

void DungeonWidget::placePlayer()
{
    if (m_player)
        m_player->setPos((WORLD_W - PW) / 2.0,
                         (WORLD_H - PH) / 2.0);
}

void DungeonWidget::spawnEnemies()
{
    // Remove any previously spawned enemies from the scene
    for (EnemySprite *e : m_enemies) {
        m_scene->removeItem(e);
        delete e;
    }
    m_enemies.clear();

    // Enemy name pool mirrors GameEngine::spawnEnemy()
    const QStringList names = {
        "Shadow Warrior", "Dark Mage", "Black Arrow",
        "Iron Fist", "Void Caster", "Silent Hunter"
    };

    auto *rng = QRandomGenerator::global();
    const int count = 4;   // number of enemies in the dungeon

    for (int i = 0; i < count; ++i) {
        CharacterType type = static_cast<CharacterType>(rng->bounded(3));
        QString name = names[rng->bounded(names.size())];

        auto *enemy = new EnemySprite(type, name);
        enemy->setZValue(8);

        // Random starting position, avoiding the centre spawn area and edges
        qreal ex, ey;
        do {
            ex = 60 + rng->bounded(WORLD_W - 120);
            ey = 80 + rng->bounded(WORLD_H - 200);
        } while (qAbs(ex - WORLD_W / 2.0) < 80 && qAbs(ey - WORLD_H / 2.0) < 80);

        enemy->setPos(ex, ey);
        m_scene->addItem(enemy);
        m_enemies.append(enemy);

        // Small name label above each enemy
        auto *lbl = m_scene->addText(name, QFont("Arial", 6));
        lbl->setDefaultTextColor(QColor("#ffcccc"));
        lbl->setPos(ex - 10, ey - 14);
        lbl->setZValue(9);
        // Note: labels are cosmetic only and don't move with enemies.
        // For moving labels, parent them to the EnemySprite item instead.
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Game loop
// ─────────────────────────────────────────────────────────────────────────────

void DungeonWidget::onTick()
{
    movePlayer();
    patrolEnemies();
    checkCollisions();
}

void DungeonWidget::movePlayer()
{
    qreal dx = 0, dy = 0;

    if (m_heldKeys.contains(Qt::Key_W) || m_heldKeys.contains(Qt::Key_Up))    dy -= SPEED;
    if (m_heldKeys.contains(Qt::Key_S) || m_heldKeys.contains(Qt::Key_Down))  dy += SPEED;
    if (m_heldKeys.contains(Qt::Key_A) || m_heldKeys.contains(Qt::Key_Left))  dx -= SPEED;
    if (m_heldKeys.contains(Qt::Key_D) || m_heldKeys.contains(Qt::Key_Right)) dx += SPEED;

    if (dx != 0 && dy != 0) { dx *= 0.7071; dy *= 0.7071; }

    qreal nx = qBound(0.0,           m_player->x() + dx, (qreal)(WORLD_W) - PW);
    qreal ny = qBound(60.0,          m_player->y() + dy, (qreal)(WORLD_H) - PH);
    m_player->setPos(nx, ny);
}

void DungeonWidget::patrolEnemies()
{
    const QRectF bounds(0, 0, WORLD_W, WORLD_H);
    for (EnemySprite *e : m_enemies)
        e->patrol(bounds);
}

void DungeonWidget::checkCollisions()
{
    // ── Enemy collision → trigger battle ─────────────────────────────────────
    for (EnemySprite *e : m_enemies) {
        if (m_player->collidesWithItem(e)) {
            deactivate();
            emit battleTriggered(e->enemyType(), e->enemyName());
            return;   // only one collision per frame
        }
    }

    // ── Exit portal → back to overworld ──────────────────────────────────────
    if (m_player->collidesWithItem(m_exitZone)) {
        deactivate();
        emit exitedDungeon();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input
// ─────────────────────────────────────────────────────────────────────────────

void DungeonWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Escape) {
        deactivate();
        emit backToMenu();
        return;
    }
    m_heldKeys.insert(e->key());
}

void DungeonWidget::keyReleaseEvent(QKeyEvent *e)
{
    m_heldKeys.remove(e->key());
}

// ─────────────────────────────────────────────────────────────────────────────
//  Resize
// ─────────────────────────────────────────────────────────────────────────────

void DungeonWidget::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);

}

void DungeonWidget::fitView()
{
 
}
