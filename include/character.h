#pragma once
#include <QString>

enum class CharacterType { Warrior, Mage, Archer };

class Character {
public:
    Character(const QString& name, int health, int attackPower,
              int maxSp, int spPerAttack, int specialCost);
    virtual ~Character();

    // ── Health ────────────────────────────────────
    QString getName()          const;
    int     getMaxHealth()     const;
    float   getHealthPercent() const;
    bool    isAlive()          const;
    void    takeDamage(int damage);
    void    heal(int amount);
    void    resetHealth();

    // ── Energy (SP) ───────────────────────────────
    int     getSp()            const;
    int     getMaxSp()         const;
    float   getSpPercent()     const;   // 0.0 – 1.0, used by energy bar widget
    bool    canUseSpecial()    const;   // sp >= m_specialCost
    void    addSp(int amount);          // called after basic attack
    void    drainSp();                  // called when special is used
    void    addSpFromAttack();   // adds m_spPerAttack — called after basic attack

    // ── Combat interface ──────────────────────────
    virtual CharacterType getType()        const = 0;
    virtual int           attack()         const = 0;
    virtual int           specialAbility() const = 0;

    // ── Static ────────────────────────────────────
    static int getCharacterCount();

protected:
    int getAttackPower() const;
    int getHealth()      const;

private:
    QString m_name;
    int     m_health;
    int     m_maxHealth;
    int     m_attackPower;

    int     m_sp;           // current stamina points
    int     m_maxSp;        // cap (always 100 in practice)
    int     m_spPerAttack;  // SP gained per basic attack
    int     m_specialCost;  // SP required to use special

    static int s_characterCount;
};
