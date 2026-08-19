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

#endif
