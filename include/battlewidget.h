#pragma once
#include <QWidget>
#include "gameengine.h"
#include "battleresult.h"
#include "audiomanager.h"
class HealthBarWidget;
class SpriteWidget;
class BattleLogWidget;
class BattleMenuWidget;
class PauseOverlayWidget;
class RoundOverWidget;
class QLabel;
class QKeyEvent;


class BattleWidget : public QWidget {
    Q_OBJECT
public:
    explicit BattleWidget(GameEngine* engine, AudioManager* audio, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void onStateChanged(GameState state);
    void onHealthUpdated(float playerPct, float enemyPct);
    void onBattleActionResolved(const BattleResult& result);
    void onRoundEnded(int pScore, int eScore, bool playerWon);
    void onEnergyUpdated(float playerPct, float enemyPct);


private:
    void layoutChildren();

    GameEngine*      m_engine;
    AudioManager* m_audio;
    // ── Battle area ───────────────────────────────
    SpriteWidget*    m_playerSprite;
    SpriteWidget*    m_enemySprite;
    HealthBarWidget* m_playerHP;
    HealthBarWidget* m_enemyHP;
    QLabel*          m_playerNameLabel;
    QLabel*          m_enemyNameLabel;
    QLabel*          m_roundLabel;
    QLabel*          m_scoreLabel;
    HealthBarWidget* m_playerSP;
    HealthBarWidget* m_enemySP;
    QLabel*          m_playerSpLabel;  // (shows "SP" text)
    QLabel*          m_enemySpLabel;

    // ── Bottom panel ──────────────────────────────
    BattleLogWidget* m_log;
    BattleMenuWidget*m_menu;

    // ── Overlays ──────────────────────────────────
    PauseOverlayWidget* m_pauseOverlay;
    RoundOverWidget*    m_roundOver;

    QPixmap m_background;
};
