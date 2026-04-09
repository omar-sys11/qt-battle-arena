#include "character.h"
#include <QDebug>
#include <algorithm>

int Character::s_characterCount = 0;

Character::Character(const QString& name, int health, int attackPower,
                     int maxSp, int spPerAttack, int specialCost)
    : m_name(name)
    , m_health(health)
    , m_maxHealth(health)
    , m_attackPower(attackPower)
    , m_sp(0)               // always starts at 0 — must earn SP first
    , m_maxSp(maxSp)
    , m_spPerAttack(spPerAttack)
    , m_specialCost(specialCost)
{
    ++s_characterCount;
}

Character::~Character()
{
    --s_characterCount;
    qDebug() << m_name << "removed from arena.";
}

// ── Health ────────────────────────────────────────────────────────────────

QString Character::getName()          const { return m_name; }
int     Character::getMaxHealth()     const { return m_maxHealth; }
int     Character::getHealth()        const { return m_health; }
int     Character::getAttackPower()   const { return m_attackPower; }
int     Character::getCharacterCount()      { return s_characterCount; }

float Character::getHealthPercent() const
{
    if (m_maxHealth == 0) return 0.0f;
    return static_cast<float>(m_health) / static_cast<float>(m_maxHealth);
}

bool Character::isAlive() const { return m_health > 0; }

void Character::takeDamage(int damage)
{
    m_health = std::max(0, m_health - damage);
}

void Character::heal(int amount)
{
    m_health = std::min(m_maxHealth, m_health + amount);
}

void Character::resetHealth()
{
    m_health = m_maxHealth;
}

// ── Energy (SP) ───────────────────────────────────────────────────────────

int   Character::getSp()        const { return m_sp; }
int   Character::getMaxSp()     const { return m_maxSp; }
bool  Character::canUseSpecial()const { return m_sp >= m_specialCost; }

float Character::getSpPercent() const
{
    if (m_maxSp == 0) return 0.0f;
    return static_cast<float>(m_sp) / static_cast<float>(m_maxSp);
}

void Character::addSp(int amount)
{
    m_sp = std::min(m_maxSp, m_sp + amount);
}

void Character::drainSp()
{
    m_sp = std::max(0, m_sp - m_specialCost);
}
void Character::addSpFromAttack()
{
    addSp(m_spPerAttack);
}
