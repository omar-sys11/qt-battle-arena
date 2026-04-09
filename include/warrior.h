#pragma once
#include "character.h"

class Warrior : public Character {
public:
    Warrior(const QString& name);
    ~Warrior() override;

    CharacterType getType()        const override;
    int           attack()         const override;
    int           specialAbility() const override; // "Power Strike"
};
