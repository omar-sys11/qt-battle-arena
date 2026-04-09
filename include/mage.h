#pragma once
#include "character.h"

class Mage : public Character {
public:
    Mage(const QString& name);
    ~Mage() override;

    CharacterType getType()        const override;
    int           attack()         const override;
    int           specialAbility() const override; // "Arcane Storm"
};
