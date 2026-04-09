#include "gameengine.h"
#include "warrior.h"
#include "mage.h"
#include "archer.h"
#include <QRandomGenerator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

GameEngine::GameEngine(QObject* parent)
    : QObject(parent)
{
    m_roundTimer = new QTimer(this);
    m_roundTimer->setSingleShot(true);
    connect(m_roundTimer, &QTimer::timeout, this, &GameEngine::enemyTakeTurn);
}

GameEngine::~GameEngine() { cleanupCharacters(); }

void GameEngine::setState(GameState s)
{
    m_state = s;
    emit stateChanged(s);
}

// kept for internal use — same as setState, just an alias so old call sites compile
void GameEngine::setState_internal(GameState s) { setState(s); }

void GameEngine::cleanupCharacters()
{
    delete m_player; m_player = nullptr;
    delete m_enemy;  m_enemy  = nullptr;
    m_allCharacters.clear();
}

Character* GameEngine::createCharacter(CharacterType type, const QString& name)
{
    switch (type) {
    case CharacterType::Warrior: return new Warrior(name);
    case CharacterType::Mage:    return new Mage(name);
    case CharacterType::Archer:  return new Archer(name);
    }
    return new Warrior(name);
}

void GameEngine::spawnEnemy()
{
    const QStringList names = {
        "Shadow Warrior", "Dark Mage", "Black Arrow",
        "Iron Fist", "Void Caster", "Silent Hunter"
    };
    CharacterType type = static_cast<CharacterType>(
        QRandomGenerator::global()->bounded(3));
    QString name = names[QRandomGenerator::global()->bounded(names.size())];
    delete m_enemy;
    m_enemy = createCharacter(type, name);
}

void GameEngine::resetRound()
{
    delete m_player;
    m_player = createCharacter(m_playerType, m_playerName);
    spawnEnemy();
    m_allCharacters.clear();
    m_allCharacters.append(m_player);
    m_allCharacters.append(m_enemy);
    m_itemUsed = false;
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
}

void GameEngine::onStartGame()  { setState(GameState::CharacterSelect); }
void GameEngine::onDifficultyChanged(Difficulty d) { m_difficulty = d; }

void GameEngine::onPlayerSelectedCharacter(CharacterType type, const QString& name)
{
    m_playerType = type;
    m_playerName = name;
    cleanupCharacters();
    m_player = createCharacter(type, name);
    spawnEnemy();
    m_allCharacters.append(m_player);
    m_allCharacters.append(m_enemy);
    m_playerScore  = 0;
    m_enemyScore   = 0;
    m_currentRound = 1;
    m_itemUsed     = false;
    m_playerMoveHistory.clear();
    m_roundHistory.clear();
    m_tracker.reset();
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(QString("Round %1 — %2 vs %3!")
                              .arg(m_currentRound)
                              .arg(m_player->getName())
                              .arg(m_enemy->getName()));
    setState(GameState::PlayerTurn);
}

void GameEngine::resolvePlayerAction(bool useSpecial)
{
    if (m_state != GameState::PlayerTurn) return;

    if (useSpecial && !m_player->canUseSpecial()) {
        emit battleLogMessage("Not enough SP for Special!");
        return;   // don't even leave PlayerTurn state
    }

    setState(GameState::AnimatingAttack);
    int damage = useSpecial ? m_player->specialAbility() : m_player->attack();
    // ── Update SP ─────────────────────────────────
    if (useSpecial)
        m_player->drainSp();
    else
        m_player->addSpFromAttack();   // ← clean, no magic number here

    emit energyUpdated(m_player->getSpPercent(), m_enemy->getSpPercent());


    emit energyUpdated(m_player->getSpPercent(), m_enemy->getSpPercent());
    m_playerMoveHistory.append(useSpecial);
    if (m_playerMoveHistory.size() > 5) m_playerMoveHistory.removeFirst();
    m_enemy->takeDamage(damage);
    BattleResult result;
    result.attackerName    = m_player->getName();
    result.defenderName    = m_enemy->getName();
    result.damageDone      = damage;
    result.usedSpecial     = useSpecial;
    result.defenderFainted = !m_enemy->isAlive();
    emit battleActionResolved(result);
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        useSpecial
            ? QString("%1 unleashes Special for %2 dmg!").arg(m_player->getName()).arg(damage)
            : QString("%1 attacks for %2 dmg!").arg(m_player->getName()).arg(damage));
    if (!m_enemy->isAlive()) {
        emit battleLogMessage(m_enemy->getName() + " fainted!");
        QTimer::singleShot(1400, this, [this]{ endRound(true); });
    } else {
        int delay = (m_difficulty == Difficulty::Hard) ? 550
                    : (m_difficulty == Difficulty::Easy) ? 1300 : 900;
        m_roundTimer->start(delay);
    }
}

void GameEngine::onPlayerChoseFight()   { resolvePlayerAction(false); }
void GameEngine::onPlayerChoseSpecial() { resolvePlayerAction(true);  }

void GameEngine::onPlayerChoseItem()
{
    if (m_itemUsed || m_state != GameState::PlayerTurn) return;
    setState(GameState::AnimatingAttack);
    m_itemUsed = true;
    int healAmount = static_cast<int>(m_player->getMaxHealth() * 0.35f);
    m_player->heal(healAmount);
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit energyUpdated(m_player->getSpPercent(), m_enemy->getSpPercent());
    emit battleLogMessage(QString("%1 used a Potion! Recovered %2 HP!")
                              .arg(m_player->getName()).arg(healAmount));
    m_roundTimer->start(900);
}

void GameEngine::onPlayerChoseRun()
{
    if (m_state != GameState::PlayerTurn) return;
    m_roundTimer->stop();
    emit battleLogMessage(m_player->getName() + " fled! Round forfeited.");
    endRound(false);
}

void GameEngine::enemyTakeTurn()
{
    if (!m_enemy->isAlive() || !m_player->isAlive()) return;
    bool useSpecial = false;
    int  damage     = 0;
    switch (m_difficulty) {
    case Difficulty::Easy:
        damage = m_enemy->attack();
        break;
    case Difficulty::Normal:
        useSpecial = QRandomGenerator::global()->bounded(100) < 25;
        damage = useSpecial ? m_enemy->specialAbility() : m_enemy->attack();
        break;
    case Difficulty::Hard: {
        bool lowHealth = m_enemy->getHealthPercent() < 0.4f;
        bool playerPredictable = false;
        if (m_playerMoveHistory.size() >= 3) {
            playerPredictable =
                !m_playerMoveHistory[m_playerMoveHistory.size()-1] &&
                !m_playerMoveHistory[m_playerMoveHistory.size()-2] &&
                !m_playerMoveHistory[m_playerMoveHistory.size()-3];
        }
        useSpecial = lowHealth || playerPredictable;
        damage = useSpecial ? m_enemy->specialAbility() : m_enemy->attack();
        break;
    }
    }
    if (useSpecial)
        m_enemy->drainSp();
    else
        m_enemy->addSpFromAttack();

    emit energyUpdated(m_player->getSpPercent(), m_enemy->getSpPercent());
    m_player->takeDamage(damage);
    BattleResult result;
    result.attackerName    = m_enemy->getName();
    result.defenderName    = m_player->getName();
    result.damageDone      = damage;
    result.usedSpecial     = useSpecial;
    result.defenderFainted = !m_player->isAlive();
    emit battleActionResolved(result);
    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        useSpecial
            ? QString("%1 unleashes Special for %2 dmg!").arg(m_enemy->getName()).arg(damage)
            : QString("%1 attacks for %2 dmg!").arg(m_enemy->getName()).arg(damage));
    if (!m_player->isAlive()) {
        emit battleLogMessage(m_player->getName() + " fainted!");
        QTimer::singleShot(1400, this, [this]{ endRound(false); });
    } else {
        setState(GameState::PlayerTurn);
    }
}

void GameEngine::endRound(bool playerWon)
{
    m_roundTimer->stop();
    if (playerWon) { ++m_playerScore; m_tracker.recordWin();  }
    else           { ++m_enemyScore;  m_tracker.recordLoss(); }

    // ── Record this round in history ──────────────
    RoundRecord rec;
    rec.roundNumber = m_currentRound;
    rec.playerWon   = playerWon;
    rec.enemyName   = m_enemy ? m_enemy->getName() : "???";
    m_roundHistory.append(rec);

    emit roundEnded(m_playerScore, m_enemyScore, playerWon);
    setState(GameState::RoundOver);

    bool isGameOver = (m_playerScore >= 3)
                      || (m_enemyScore  >= 3)
                      || (m_currentRound >= m_maxRounds);

    if (isGameOver) {
        QTimer::singleShot(2200, this, [this]{
            setState(GameState::GameOver);
            emit gameOver(m_playerScore > m_enemyScore,
                          m_playerScore, m_enemyScore);
        });
    } else {
        ++m_currentRound;
        QTimer::singleShot(2500, this, [this]{
            resetRound();
            emit battleLogMessage(
                QString("Round %1 — Fight!").arg(m_currentRound));
            setState(GameState::PlayerTurn);
        });
    }
}

void GameEngine::onPauseToggle()
{
    if (m_state == GameState::PlayerTurn || m_state == GameState::Playing) {
        m_roundTimer->stop();
        setState(GameState::Paused);
    } else if (m_state == GameState::Paused) {
        setState(GameState::PlayerTurn);
    }
}

void GameEngine::onRestartGame()
{
    m_roundTimer->stop();
    cleanupCharacters();
    m_playerScore  = 0;
    m_enemyScore   = 0;
    m_currentRound = 1;
    m_itemUsed     = false;
    m_playerMoveHistory.clear();
    m_roundHistory.clear();
    m_tracker.reset();
    setState(GameState::MainMenu);
}

void GameEngine::onExitToMenu() { onRestartGame(); }

// ── Save: stores full mid-battle state ────────────────────────────────────
bool GameEngine::onSaveGame(const QString& path)
{
    // Only save when there is actually a battle in progress
    if (!m_player || !m_enemy) return false;
    if (m_state != GameState::PlayerTurn && m_state != GameState::Paused)
        return false;

    QJsonObject obj;
    obj["playerName"]     = m_playerName;
    obj["playerType"]     = static_cast<int>(m_playerType);
    obj["playerHealth"]   = static_cast<int>(
        m_player->getHealthPercent() * m_player->getMaxHealth());
    obj["playerMaxHealth"]= m_player->getMaxHealth();
    obj["playerSp"] = m_player->getSp();

    obj["enemyName"]      = m_enemy->getName();
    obj["enemyType"]      = static_cast<int>(m_enemy->getType());
    obj["enemyHealth"]    = static_cast<int>(
        m_enemy->getHealthPercent() * m_enemy->getMaxHealth());
    obj["enemyMaxHealth"] = m_enemy->getMaxHealth();
    obj["enemySp"]  = m_enemy->getSp();

    obj["currentRound"]   = m_currentRound;
    obj["playerScore"]    = m_playerScore;
    obj["enemyScore"]     = m_enemyScore;
    obj["itemUsed"]       = m_itemUsed;
    obj["difficulty"]     = static_cast<int>(m_difficulty);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(obj).toJson());
    return true;
}

// ── Load: restores mid-battle state and jumps straight to PlayerTurn ──────
bool GameEngine::onLoadGame(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    // Basic validation — if key fields are missing the file is corrupt
    if (!obj.contains("playerName") || !obj.contains("enemyName"))
        return false;

    m_roundTimer->stop();
    cleanupCharacters();
    m_roundHistory.clear();
    m_playerMoveHistory.clear();

    m_playerName   = obj["playerName"].toString();
    m_playerType   = static_cast<CharacterType>(obj["playerType"].toInt());
    m_difficulty   = static_cast<Difficulty>(obj["difficulty"].toInt());
    m_currentRound = obj["currentRound"].toInt(1);
    m_playerScore  = obj["playerScore"].toInt(0);
    m_enemyScore   = obj["enemyScore"].toInt(0);
    m_itemUsed     = obj["itemUsed"].toBool(false);


    // Recreate player at full health then apply saved damage
    m_player = createCharacter(m_playerType, m_playerName);
    int savedPlayerHP  = obj["playerHealth"].toInt(m_player->getMaxHealth());
    int playerDamage   = m_player->getMaxHealth() - savedPlayerHP;
    if (playerDamage > 0) m_player->takeDamage(playerDamage);
    int savedPlayerSp = obj["playerSp"].toInt(0);
    m_player->addSp(savedPlayerSp);

    // Recreate enemy
    CharacterType enemyType = static_cast<CharacterType>(obj["enemyType"].toInt());
    QString       enemyName = obj["enemyName"].toString();
    m_enemy = createCharacter(enemyType, enemyName);
    int savedEnemyHP = obj["enemyHealth"].toInt(m_enemy->getMaxHealth());
    int enemyDamage  = m_enemy->getMaxHealth() - savedEnemyHP;
    if (enemyDamage > 0) m_enemy->takeDamage(enemyDamage);
    int savedEnemySp = obj["enemySp"].toInt(0);
    m_enemy->addSp(savedEnemySp);

    m_allCharacters.append(m_player);
    m_allCharacters.append(m_enemy);

    emit healthUpdated(m_player->getHealthPercent(), m_enemy->getHealthPercent());
    emit battleLogMessage(
        QString("Save loaded! Round %1 — %2 vs %3. Continue!")
            .arg(m_currentRound)
            .arg(m_playerName)
            .arg(enemyName));

    // Jump directly into the battle — no character select needed
    setState(GameState::PlayerTurn);
    return true;
}
