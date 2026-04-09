#pragma once
#include "character.h"

class Archer : public Character {
public:
    Archer(const QString& name);
    ~Archer() override;

    CharacterType getType()        const override;
    int           attack()         const override;
    int           specialAbility() const override; // "Double Shot"
};
