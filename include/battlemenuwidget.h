#pragma once
#include <QWidget>
#include "gameengine.h"

class QPushButton;
class QKeyEvent;        // ← ADD THIS

class BattleMenuWidget : public QWidget {
    Q_OBJECT
public:
    explicit BattleMenuWidget(GameEngine* engine, QWidget* parent = nullptr);

    void setMenuEnabled(bool enabled);
    void updateItemButton();        // greys it out after use
    void updateSpecialButton();
protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    GameEngine*  m_engine;
    QPushButton* m_buttons[4];      // 0=Fight 1=Special 2=Item 3=Run
    int          m_cursorIndex = 0;

    // cursor order: 0(Fight) 1(Special) 2(Item) 3(Run)
    // Arrow navigation: Left/Right toggles column, Up/Down toggles row
    void moveCursor(int newIndex);
    void confirmSelection();
    void updateCursor();
};
