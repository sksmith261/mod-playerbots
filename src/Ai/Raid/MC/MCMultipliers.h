#ifndef PLAYERBOTS_MCMULTIPLIERS_H
#define PLAYERBOTS_MCMULTIPLIERS_H

#include "Multiplier.h"

// Majordomo rotates Magic/Damage Reflection shields onto his adds. DPS bots
// whose current target carries the wrong shield stop feeding it damage to
// reflect (the kill-order mark steers them to clean targets; this is the
// backstop for the window before they switch).
class MajordomoReflectionMultiplier : public Multiplier
{
public:
    MajordomoReflectionMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "majordomo reflection multiplier") {}
    float GetValue(Action* action) override;
};

class GolemaggMultiplier : public Multiplier
{
public:
    GolemaggMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "golemagg multiplier") {}
    float GetValue(Action* action) override;
};

#endif
