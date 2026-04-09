// include/mainwindow.h
#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "gameengine.h"
#include "audiomanager.h"
#include "character.h"   // for CharacterType

// Forward declarations
class StartScreenWidget;
class CharacterSelectWidget;
class BattleWidget;
class GameOverWidget;
class ScoreboardWidget;
class OverworldWidget;
class DungeonWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    // Driven by GameEngine::stateChanged (battle states)
    void onStateChanged(GameState newState);

    // Driven by OverworldWidget / DungeonWidget signals
    void onDungeonEntered();
    void onExitedDungeon();
    void onBattleTriggered(CharacterType enemyType, const QString &enemyName);
    void onBackToMenu();

private:
    void buildUI();
    void buildMenuBar();

    GameEngine*            m_engine        = nullptr;
    QStackedWidget*        m_stack         = nullptr;
    AudioManager*          m_audio         = nullptr;

    // ── Screens ───────────────────────────────────────────────────────────────
    StartScreenWidget*     m_startScreen   = nullptr;
    CharacterSelectWidget* m_charSelect    = nullptr;
    OverworldWidget*       m_overworld     = nullptr;
    DungeonWidget*         m_dungeon       = nullptr;
    BattleWidget*          m_battleWidget  = nullptr;
    GameOverWidget*        m_gameOver      = nullptr;
    ScoreboardWidget*      m_scoreboard    = nullptr;

    // Remembered so MainWindow can start the fight after character select
    CharacterType m_pendingEnemyType = CharacterType::Warrior;
    QString       m_pendingEnemyName;
};
