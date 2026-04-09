#include "scoreboardwidget.h"
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>

ScoreboardWidget::ScoreboardWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setAlignment(Qt::AlignCenter);
    root->setSpacing(12);

    // ── Match result header ───────────────────────
    m_winnerLabel = new QLabel(this);
    m_winnerLabel->setObjectName("titleLabel");
    m_winnerLabel->setAlignment(Qt::AlignCenter);

    m_scoreLabel = new QLabel(this);
    m_scoreLabel->setObjectName("subtitleLabel");
    m_scoreLabel->setAlignment(Qt::AlignCenter);

    // ── Round history table ───────────────────────
    QLabel* roundTitle = new QLabel("ROUND HISTORY", this);
    roundTitle->setObjectName("subtitleLabel");
    roundTitle->setAlignment(Qt::AlignCenter);

    m_roundTable = new QTableWidget(this);
    m_roundTable->setColumnCount(3);
    m_roundTable->setHorizontalHeaderLabels({"ROUND", "OPPONENT", "RESULT"});
    m_roundTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_roundTable->verticalHeader()->setVisible(false);
    m_roundTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_roundTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_roundTable->setFixedHeight(160);
    m_roundTable->setMinimumWidth(500);

    // ── Character summary table ───────────────────
    QLabel* charTitle = new QLabel("FIGHTERS", this);
    charTitle->setObjectName("subtitleLabel");
    charTitle->setAlignment(Qt::AlignCenter);

    m_charTable = new QTableWidget(this);
    m_charTable->setColumnCount(4);
    m_charTable->setHorizontalHeaderLabels({"NAME", "TYPE", "HP LEFT", "STATUS"});
    m_charTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_charTable->verticalHeader()->setVisible(false);
    m_charTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_charTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_charTable->setFixedHeight(90);
    m_charTable->setMinimumWidth(500);

    // ── Buttons ───────────────────────────────────
    QHBoxLayout* btnRow = new QHBoxLayout;
    btnRow->setAlignment(Qt::AlignCenter);
    QPushButton* playAgain = new QPushButton("► PLAY AGAIN", this);
    QPushButton* menuBtn   = new QPushButton("  MAIN MENU",  this);
    connect(playAgain, &QPushButton::clicked, engine, &GameEngine::onRestartGame);
    connect(menuBtn,   &QPushButton::clicked, engine, &GameEngine::onExitToMenu);
    btnRow->addWidget(playAgain);
    btnRow->addWidget(menuBtn);

    connect(engine, &GameEngine::stateChanged,
            this,   &ScoreboardWidget::onStateChanged);

    root->addStretch(1);
    root->addWidget(m_winnerLabel);
    root->addWidget(m_scoreLabel);
    root->addSpacing(12);
    root->addWidget(roundTitle);
    root->addWidget(m_roundTable, 0, Qt::AlignCenter);
    root->addSpacing(8);
    root->addWidget(charTitle);
    root->addWidget(m_charTable, 0, Qt::AlignCenter);
    root->addSpacing(16);
    root->addLayout(btnRow);
    root->addStretch(1);
}

void ScoreboardWidget::onStateChanged(GameState state)
{
    if (state == GameState::Scoreboard) refresh();
}

void ScoreboardWidget::refresh()
{
    int pScore = m_engine->getPlayerScore();
    int eScore = m_engine->getEnemyScore();

    // ── Winner label ──────────────────────────────
    if (pScore > eScore)
        m_winnerLabel->setText("✦ " + m_engine->getPlayerName() + " WINS! ✦");
    else if (eScore > pScore)
        m_winnerLabel->setText("✦ CPU WINS ✦");
    else
        m_winnerLabel->setText("✦ DRAW ✦");

    m_scoreLabel->setText(
        QString("%1   %2 — %3   CPU")
            .arg(m_engine->getPlayerName())
            .arg(pScore)
            .arg(eScore));

    // ── Round history ─────────────────────────────
    const auto& history = m_engine->getRoundHistory();
    m_roundTable->setRowCount(history.size());

    for (int i = 0; i < history.size(); ++i) {
        const auto& rec = history[i];

        auto* numItem    = new QTableWidgetItem(QString("Round %1").arg(rec.roundNumber));
        auto* enemyItem  = new QTableWidgetItem(rec.enemyName);
        auto* resultItem = new QTableWidgetItem(rec.playerWon ? "WIN" : "LOSS");

        numItem->setTextAlignment(Qt::AlignCenter);
        enemyItem->setTextAlignment(Qt::AlignCenter);
        resultItem->setTextAlignment(Qt::AlignCenter);
        resultItem->setForeground(rec.playerWon ? QColor("#44BB44") : QColor("#EE3333"));

        m_roundTable->setItem(i, 0, numItem);
        m_roundTable->setItem(i, 1, enemyItem);
        m_roundTable->setItem(i, 2, resultItem);
    }

    // ── Character summary ─────────────────────────
    auto typeName = [](CharacterType t) -> QString {
        switch(t) {
        case CharacterType::Warrior: return "WARRIOR";
        case CharacterType::Mage:    return "MAGE";
        case CharacterType::Archer:  return "ARCHER";
        }
        return "?";
    };

    const auto& chars = m_engine->getAllCharacters();
    m_charTable->setRowCount(chars.size());
    for (int i = 0; i < chars.size(); ++i) {
        auto* c = chars[i];
        int hpLeft = static_cast<int>(c->getHealthPercent() * c->getMaxHealth());

        auto* nameItem = new QTableWidgetItem(c->getName());
        auto* typeItem = new QTableWidgetItem(typeName(c->getType()));
        auto* hpItem   = new QTableWidgetItem(
            QString("%1 / %2").arg(hpLeft).arg(c->getMaxHealth()));
        auto* statusItem = new QTableWidgetItem(c->isAlive() ? "ALIVE" : "DEFEATED");

        nameItem->setTextAlignment(Qt::AlignCenter);
        typeItem->setTextAlignment(Qt::AlignCenter);
        hpItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(c->isAlive() ? QColor("#44BB44") : QColor("#EE3333"));

        m_charTable->setItem(i, 0, nameItem);
        m_charTable->setItem(i, 1, typeItem);
        m_charTable->setItem(i, 2, hpItem);
        m_charTable->setItem(i, 3, statusItem);
    }
}

void ScoreboardWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPixmap bg(":/backgrounds/battle_bg.png");
    if (!bg.isNull()) p.drawPixmap(rect(), bg);
    else p.fillRect(rect(), QColor("#1A1A2E"));
    p.fillRect(rect(), QColor(0, 0, 0, 150));
}
