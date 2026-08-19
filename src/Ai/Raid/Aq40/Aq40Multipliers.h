#ifndef PLAYERBOTS_AQ40MULTIPLIERS_H
#define PLAYERBOTS_AQ40MULTIPLIERS_H

#include "Multiplier.h"

class PlayerbotAI;

// Twin Emperors duty enforcement: ANY action aimed at the twin this bot's
// damage type cannot hurt is multiplied to zero. This is what finally ends
// the assist-logic tug-of-war — the yank-back impulses (master's target,
// raid assist, default dps target) aren't out-argued, they're deleted.
class Aq40TwinsDutyMultiplier : public Multiplier
{
public:
    Aq40TwinsDutyMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "aq40 twins duty") {}
    float GetValue(Action* action) override;
};

// C'Thun P1: the eye needs no tank and melee cannot usefully reach it —
// any melee bot's action aimed at the Eye of C'Thun is zeroed, so assist
// logic can't drag the melee half of the raid off the ring into the
// beam/glare kill zone. Ranged and healers are untouched; P2 (eye dead)
// is untouched.
class Aq40CthunP1MeleeMultiplier : public Multiplier
{
public:
    Aq40CthunP1MeleeMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "aq40 cthun p1 melee") {}
    float GetValue(Action* action) override;
};

#endif
