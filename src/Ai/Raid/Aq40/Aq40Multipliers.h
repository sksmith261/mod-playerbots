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

// C'Thun stomach focus: a swallowed bot still sees raid icons and assist
// targets up in the chamber (icons are instance-global), so its casts on
// the Flesh Tentacle kept getting retargeted and cancelled. While the bot
// is in the stomach AND a Flesh Tentacle lives, any action aimed at a unit
// above the stomach is zeroed. The stomach action re-asserts the tentacle
// as current target every tick at ACTION_RAID (an inside target — never
// zeroed), and the multiplier disarms once the tentacles are dead, so the
// exit run can never be blocked (the paralysis lesson, applied).
class Aq40StomachFocusMultiplier : public Multiplier
{
public:
    Aq40StomachFocusMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "aq40 stomach focus") {}
    float GetValue(Action* action) override;
};

#endif
