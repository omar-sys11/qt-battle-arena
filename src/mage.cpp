#include "mage.h"
#include <QRandomGenerator>

// ── Rebalance these freely ─────────────────────────────────────────────────
static constexpr int MAGE_BASE_HEALTH   = 80;
static constexpr int MAGE_BASE_ATTACK   = 35;
static constexpr int MAGE_MAX_SP        = 100;
static constexpr int MAGE_SP_PER_ATTACK = 15;   // slowest buildup
static constexpr int MAGE_SPECIAL_COST  = 80;   // most expensive — huge payoff
// ──────────────────────────────────────────────────────────────────────────

Mage::Mage(const QString& name)
    : Character(name,
                MAGE_BASE_HEALTH,
                MAGE_BASE_ATTACK,
                MAGE_MAX_SP,
                MAGE_SP_PER_ATTACK,
                MAGE_SPECIAL_COST)
{}

Mage::~Mage() = default;

CharacterType Mage::getType() const { return CharacterType::Mage; }

int Mage::attack() const
{
    int base      = getAttackPower();
    int variation = QRandomGenerator::global()->bounded(-8, 13);
    return std::max(0, base + variation);
}

int Mage::specialAbility() const
{
    // Arcane Storm: massive, high-variance hit
    int base  = getAttackPower() * 2;
    int storm = QRandomGenerator::global()->bounded(0, 31);
    return base + storm;
}
