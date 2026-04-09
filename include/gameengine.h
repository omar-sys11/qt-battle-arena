#pragma once
#include <QObject>
#include <QTimer>
#include "character.h"
#include "battleresult.h"
#include "scoretracker.h"

enum class GameState {
    MainMenu, CharacterSelect, Playing, PlayerTurn,
    AnimatingAttack, Paused, RoundOver, GameOver, Scoreboard
};

enum class Difficulty { Easy, Normal, Hard };

struct RoundRecord {
    int  roundNumber;
    bool playerWon;
    QString enemyName;
};

class GameEngine : public QObject {
    Q_OBJECT

public:
    explicit GameEngine(QObject* parent = nullptr);
    ~GameEngine() override;

    GameState    getState()        const { return m_state; }
    Difficulty   getDifficulty()   const { return m_difficulty; }
    int          getPlayerScore()  const { return m_playerScore; }
    int          getEnemyScore()   const { return m_enemyScore; }
    int          getCurrentRound() const { return m_currentRound; }
    int          getMaxRounds()    const { return m_maxRounds; }
    bool         itemAvailable()   const { return !m_itemUsed; }
    SessionStats getSessionStats() const { return m_tracker.getStats(); }
    QString      getPlayerName()   const { return m_playerName; }
    CharacterType getPlayerType()  const { return m_playerType; }

    const QList<Character*>&   getAllCharacters()  const { return m_allCharacters; }
    const QList<RoundRecord>&  getRoundHistory()   const { return m_roundHistory; }

signals:
    void stateChanged(GameState newState);
    void healthUpdated(float playerPercent, float enemyPercent);
    void battleLogMessage(const QString& message);
    void battleActionResolved(BattleResult result);
    void roundEnded(int playerScore, int enemyScore, bool playerWonRound);
    void gameOver(bool playerWon, int finalPlayerScore, int finalEnemyScore);

public slots:
    void setState(GameState s);
    void onStartGame();
    void onDifficultyChanged(Difficulty d);
    void onPlayerSelectedCharacter(CharacterType type, const QString& name);
    void onPlayerChoseFight();
    void onPlayerChoseSpecial();
    void onPlayerChoseItem();
    void onPlayerChoseRun();
    void onPauseToggle();
    void onRestartGame();
    void onExitToMenu();
    bool onSaveGame(const QString& path);
    bool onLoadGame(const QString& path);

private slots:
    void enemyTakeTurn();

private:
    void setState_internal(GameState s);
    void resolvePlayerAction(bool useSpecial);
    void spawnEnemy();
    void endRound(bool playerWon);
    void cleanupCharacters();
    void resetRound();
    Character* createCharacter(CharacterType type, const QString& name);

    GameState     m_state        = GameState::MainMenu;
    Difficulty    m_difficulty   = Difficulty::Normal;
    Character*    m_player       = nullptr;
    Character*    m_enemy        = nullptr;
    QList<Character*>  m_allCharacters;
    QList<RoundRecord> m_roundHistory;
    QTimer*       m_roundTimer   = nullptr;
    int           m_playerScore  = 0;
    int           m_enemyScore   = 0;
    int           m_currentRound = 0;
    int           m_maxRounds    = 5;
    bool          m_itemUsed     = false;
    QList<bool>   m_playerMoveHistory;
    ScoreTracker  m_tracker;
    CharacterType m_playerType   = CharacterType::Warrior;
    QString       m_playerName   = "Player";
};
Q_DECLARE_METATYPE(Difficulty)
