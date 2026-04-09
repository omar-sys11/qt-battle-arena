#pragma once
#include <QWidget>
#include "gameengine.h"

class QTableWidget;
class QLabel;

class ScoreboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScoreboardWidget(GameEngine* engine, QWidget* parent = nullptr);

private slots:
    void onStateChanged(GameState state);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void refresh();
    GameEngine*   m_engine;
    QLabel*       m_winnerLabel = nullptr;
    QLabel*       m_scoreLabel  = nullptr;
    QTableWidget* m_roundTable  = nullptr;
    QTableWidget* m_charTable   = nullptr;
};
