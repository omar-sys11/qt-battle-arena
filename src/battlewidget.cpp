#include "battlewidget.h"
#include "healthbarwidget.h"
#include "spritewidget.h"
#include "battlelogwidget.h"
#include "battlemenuwidget.h"
#include "pauseoverlaywidget.h"
#include "roundoverwidget.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QTimer>               // ← ADD (for QTimer::singleShot)

BattleWidget::BattleWidget(GameEngine* engine, AudioManager* audio, QWidget* parent)
    : QWidget(parent), m_engine(engine), m_audio(audio)
{
    m_background = QPixmap(":/backgrounds/battle_bg.png");

    // ── Sprites ───────────────────────────────────
    m_playerSprite = new SpriteWidget(this);
    m_playerSprite->setFixedSize(160, 160);

    m_enemySprite = new SpriteWidget(this);
    m_enemySprite->setFixedSize(160, 160);
    m_enemySprite->setMirrored(true);

    // ── HP bars + name labels ─────────────────────
    m_playerNameLabel = new QLabel("PLAYER", this);
    m_enemyNameLabel  = new QLabel("ENEMY",  this);
    m_playerNameLabel->setObjectName("battleName");
    m_enemyNameLabel->setObjectName("battleName");

    m_playerHP = new HealthBarWidget(this);
    m_enemyHP  = new HealthBarWidget(this);
    m_playerHP->setFixedSize(200, 16);
    m_enemyHP->setFixedSize(200, 16);
    m_playerSP = new HealthBarWidget(this);
    m_enemySP  = new HealthBarWidget(this);
    m_playerSP->setFixedSize(200, 10);
    m_enemySP->setFixedSize(200, 10);
    m_playerSP->setFixedBarColor(QColor("#4A90D9"));   // blue
    m_enemySP->setFixedBarColor(QColor("#4A90D9"));
    m_playerSP->animateTo(0.0f);   // starts empty
    m_enemySP->animateTo(0.0f);

    m_playerSpLabel = new QLabel("SP", this);
    m_enemySpLabel  = new QLabel("SP", this);
    m_playerSpLabel->setObjectName("subtitleLabel");
    m_enemySpLabel->setObjectName("subtitleLabel");

    m_roundLabel = new QLabel("Round 1 / 5", this);
    m_scoreLabel = new QLabel("0 — 0",       this);
    m_roundLabel->setObjectName("subtitleLabel");
    m_scoreLabel->setObjectName("subtitleLabel");

    // ── Bottom panel ──────────────────────────────
    QWidget* bottomPanel = new QWidget(this);
    bottomPanel->setObjectName("bottomPanel");
    bottomPanel->setFixedHeight(200);

    m_log  = new BattleLogWidget(bottomPanel);
    m_menu = new BattleMenuWidget(engine, bottomPanel);

    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(8, 8, 8, 8);
    bottomLayout->setSpacing(8);
    bottomLayout->addWidget(m_log,  3);
    bottomLayout->addWidget(m_menu, 2);

    // ── Overlays (on top of everything) ──────────
    m_pauseOverlay = new PauseOverlayWidget(engine, this);
    m_roundOver    = new RoundOverWidget(this);
    m_pauseOverlay->hide();
    m_roundOver->hide();

    // ── Main layout (battle area + bottom panel) ──
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addStretch(1);
    root->addWidget(bottomPanel, 0, Qt::AlignBottom);

    // ── Connect to engine signals ─────────────────
    connect(engine, &GameEngine::stateChanged,
            this,   &BattleWidget::onStateChanged);

    connect(engine, &GameEngine::healthUpdated,
            this,   &BattleWidget::onHealthUpdated);

    connect(engine, &GameEngine::battleActionResolved,
            this,   &BattleWidget::onBattleActionResolved);

    connect(engine, &GameEngine::battleLogMessage,
            m_log,  &BattleLogWidget::appendMessage);

    connect(engine, &GameEngine::roundEnded,
            this,   &BattleWidget::onRoundEnded);

    connect(engine, &GameEngine::stateChanged, this, [this](GameState s){
        // Refresh sprite + labels whenever we enter battle
        if (s == GameState::PlayerTurn) {
            const auto& chars = m_engine->getAllCharacters();
            if (chars.size() >= 2) {
                auto* player = chars[0];
                auto* enemy  = chars[1];

                m_playerNameLabel->setText(player->getName());
                m_enemyNameLabel->setText(enemy->getName());

                // Load sprites based on type
                auto spritePath = [](CharacterType t, bool back) -> QString {
                    switch (t) {
                    case CharacterType::Warrior:
                        return back ? ":/sprites/warrior_back.png"
                                    : ":/sprites/warrior_front.png";
                    case CharacterType::Mage:
                        return back ? ":/sprites/mage_back.png"
                                    : ":/sprites/mage_front.png";
                    case CharacterType::Archer:
                        return back ? ":/sprites/archer_back.png"
                                    : ":/sprites/archer_front.png";
                    }
                    return {};
                };

                m_playerSprite->setSprite(spritePath(player->getType(), true));
                m_enemySprite->setSprite(spritePath(enemy->getType(), false));

                m_playerSprite->setFallbackColor(QColor("#4A6FA5"),
                                                 player->getName());
                m_enemySprite->setFallbackColor(QColor("#A54A4A"),
                                                enemy->getName());
            }
        }
    });
    connect(engine, &GameEngine::energyUpdated,
            this,   &BattleWidget::onEnergyUpdated);
}
void BattleWidget::onEnergyUpdated(float playerPct, float enemyPct)
{
    m_playerSP->animateTo(playerPct);
    m_enemySP->animateTo(enemyPct);
    // Also refresh SPECIAL button state
    m_menu->updateSpecialButton();
}
void BattleWidget::onStateChanged(GameState state)
{
    m_menu->setMenuEnabled(state == GameState::PlayerTurn);

    if (state == GameState::PlayerTurn) {
        m_menu->setFocus();              // ← ADD — force focus to menu
        m_menu->raise();                 // ← ADD — bring above other widgets
    }

    m_pauseOverlay->setVisible(state == GameState::Paused);
    m_roundOver->setVisible(state == GameState::RoundOver);

    m_menu->setMenuEnabled(state == GameState::PlayerTurn);
    m_pauseOverlay->setVisible(state == GameState::Paused);
    m_roundOver->setVisible(state == GameState::RoundOver);

    if (state == GameState::PlayerTurn || state == GameState::AnimatingAttack) {
        m_roundLabel->setText(
            QString("Round %1 / %2")
                .arg(m_engine->getCurrentRound())
                .arg(m_engine->getMaxRounds()));
        m_scoreLabel->setText(
            QString("%1 — %2")
                .arg(m_engine->getPlayerScore())
                .arg(m_engine->getEnemyScore()));
    }
}

void BattleWidget::onHealthUpdated(float playerPct, float enemyPct)
{
    m_playerHP->animateTo(playerPct);
    m_enemyHP->animateTo(enemyPct);
}

void BattleWidget::onBattleActionResolved(const BattleResult& result)
{
    bool playerAttacked = (result.attackerName == m_engine->getAllCharacters()[0]->getName());
    if (result.usedSpecial)
        m_audio->playSfx("/sfx/special.wav");   // ← special sound
    else
        m_audio->playSfx("/sfx/hit.wav");        // ← basic hit sound
    if (playerAttacked) {
        m_playerSprite->playAttackAnimation(true);
        QTimer::singleShot(180, this, [this, result]{
            m_enemySprite->playHitAnimation();
            if (result.defenderFainted) {
                m_enemySprite->playFaintAnimation();
                m_audio->playSfx("/sfx/faint.wav");
            }
        });
    } else {
        m_enemySprite->playAttackAnimation(false);
        QTimer::singleShot(180, this, [this, result]{
            m_playerSprite->playHitAnimation();
            if (result.defenderFainted) {
                m_playerSprite->playFaintAnimation();
                m_audio->playSfx("/sfx/faint.wav");
            }
        });
    }
}

void BattleWidget::onRoundEnded(int pScore, int eScore, bool playerWon)
{
    m_scoreLabel->setText(QString("%1 — %2").arg(pScore).arg(eScore));
    m_roundOver->show(playerWon ? "YOU WIN THE ROUND!" : "YOU LOST THE ROUND!");
}

void BattleWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!m_background.isNull())
        p.drawPixmap(rect(), m_background);
    else
        p.fillRect(rect(), QColor("#1A1A2E"));
}

void BattleWidget::resizeEvent(QResizeEvent* e)
{
    QWidget::resizeEvent(e);
    layoutChildren();

    // Keep overlays fullscreen
    if (m_pauseOverlay) m_pauseOverlay->resize(size());
    if (m_roundOver)    m_roundOver->resize(size());
}

void BattleWidget::layoutChildren()
{
    // Battle area is everything above the 200px bottom panel
    int battleH = height() - 200;
    int w       = width();

    // Enemy info: top-left
    m_enemyNameLabel->move(24, 16);
    m_enemyNameLabel->resize(200, 20);
    m_enemyHP->move(24, 40);
    m_enemySpLabel->move(6, 58);
    m_enemySP->move(24, 58);

    // Enemy sprite: upper-right
    m_enemySprite->move(w - 220, 20);

    // Player sprite: lower-left
    m_playerSprite->move(80, battleH - 180);

    // Player info: lower-right of battle area
    m_playerNameLabel->move(w - 260, battleH - 60);
    m_playerNameLabel->resize(200, 20);
    m_playerHP->move(w - 260, battleH - 36);
    m_playerSpLabel->move(w - 278, battleH - 18);
    m_playerSP->move(w - 260, battleH - 18);

    // Round/score: top-center
    m_roundLabel->move(w/2 - 80, 16);
    m_roundLabel->resize(160, 20);
    m_scoreLabel->move(w/2 - 40, 38);
    m_scoreLabel->resize(80, 20);
}
void BattleWidget::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape)
        m_engine->onPauseToggle();
    else
        QWidget::keyPressEvent(event);
}
