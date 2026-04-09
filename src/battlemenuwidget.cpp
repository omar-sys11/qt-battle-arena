#include "battlemenuwidget.h"
#include <QPushButton>
#include <QGridLayout>
#include <QKeyEvent>

BattleMenuWidget::BattleMenuWidget(GameEngine* engine, QWidget* parent)
    : QWidget(parent), m_engine(engine)
{
    setFocusPolicy(Qt::StrongFocus);

    QGridLayout* grid = new QGridLayout(this);
    grid->setSpacing(4);

    const QStringList labels = {"FIGHT", "SPECIAL", "ITEM", "RUN"};
    for (int i = 0; i < 4; ++i) {
        m_buttons[i] = new QPushButton(labels[i], this);
        m_buttons[i]->setFocusPolicy(Qt::NoFocus);
        grid->addWidget(m_buttons[i], i / 2, i % 2);
    }

    // Mouse clicks
    connect(m_buttons[0], &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseFight);
    connect(m_buttons[1], &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseSpecial);
    connect(m_buttons[2], &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseItem);
    connect(m_buttons[3], &QPushButton::clicked, m_engine, &GameEngine::onPlayerChoseRun);

    updateCursor();
}

void BattleMenuWidget::setMenuEnabled(bool enabled)
{
    for (auto* btn : m_buttons) btn->setEnabled(enabled);
    if (enabled) {
        setFocus();
        updateItemButton();
        updateSpecialButton();
    }
}

void BattleMenuWidget::updateItemButton()
{
    bool available = m_engine->itemAvailable();
    m_buttons[2]->setEnabled(available);
    m_buttons[2]->setText(available ? "ITEM" : "ITEM ✗");
}

void BattleMenuWidget::moveCursor(int newIndex)
{
    m_cursorIndex = newIndex;
    updateCursor();
}

void BattleMenuWidget::updateCursor()
{
    const QStringList base = {"FIGHT", "SPECIAL", "ITEM", "RUN"};
    for (int i = 0; i < 4; ++i) {
        QString label = (i == m_cursorIndex ? "► " : "  ") + base[i];
        if (i == 2 && !m_engine->itemAvailable()) label = "  ITEM ✗";
        m_buttons[i]->setText(label);
    }
}

void BattleMenuWidget::confirmSelection()
{
    switch (m_cursorIndex) {
    case 0: m_engine->onPlayerChoseFight();   break;
    case 1: m_engine->onPlayerChoseSpecial(); break;
    case 2: m_engine->onPlayerChoseItem();    break;
    case 3: m_engine->onPlayerChoseRun();     break;
    }
}

void BattleMenuWidget::keyPressEvent(QKeyEvent* event)
{
    // Grid layout:  [0 Fight][1 Special]
    //               [2 Item ][3 Run    ]
    switch (event->key()) {
    case Qt::Key_Left:  moveCursor(m_cursorIndex % 2 == 1 ? m_cursorIndex - 1 : m_cursorIndex); break;
    case Qt::Key_Right: moveCursor(m_cursorIndex % 2 == 0 ? m_cursorIndex + 1 : m_cursorIndex); break;
    case Qt::Key_Up:    moveCursor(m_cursorIndex >= 2     ? m_cursorIndex - 2 : m_cursorIndex); break;
    case Qt::Key_Down:  moveCursor(m_cursorIndex < 2      ? m_cursorIndex + 2 : m_cursorIndex); break;
    case Qt::Key_Return:
    case Qt::Key_Space: confirmSelection(); break;
    default: QWidget::keyPressEvent(event);
    }
}

void BattleMenuWidget::updateSpecialButton()
{
    bool canUse = m_engine->playerCanUseSpecial();
    m_buttons[1]->setEnabled(canUse);
    // The cursor label update handles the ► prefix already
    // but we need to update the text for the disabled state:
    if (!canUse)
        m_buttons[1]->setText("  SPECIAL —");
    else
        updateCursor();   // restores normal label with cursor
}
