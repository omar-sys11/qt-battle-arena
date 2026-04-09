#include "archer.h"
#include <QRandomGenerator>

// ── Rebalance these freely ─────────────────────────────────────────────────
static constexpr int ARCHER_BASE_HEALTH   = 110;
static constexpr int ARCHER_BASE_ATTACK   = 25;
static constexpr int ARCHER_MAX_SP        = 100;
static constexpr int ARCHER_SP_PER_ATTACK = 20;   // balanced buildup
static constexpr int ARCHER_SPECIAL_COST  = 50;   // cheapest — most frequent use
// ──────────────────────────────────────────────────────────────────────────

Archer::Archer(const QString& name)
    : Character(name,
                ARCHER_BASE_HEALTH,
                ARCHER_BASE_ATTACK,
                ARCHER_MAX_SP,
                ARCHER_SP_PER_ATTACK,
                ARCHER_SPECIAL_COST)
{}

Archer::~Archer() = default;

CharacterType Archer::getType() const { return CharacterType::Archer; }

int Archer::attack() const
{
    int base      = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-3, 6);
    return base + variation;
}

int Archer::specialAbility() const
{
    // Double Shot: two independent hits
    int shot1 = getAttackPower() + QRandomGenerator::global()->bounded(0, 11);
    int shot2 = getAttackPower() + QRandomGenerator::global()->bounded(0, 11);
    return shot1 + shot2;
}
