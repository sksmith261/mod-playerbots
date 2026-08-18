#ifndef PLAYERBOTS_MCMULTIPLIERS_H
#define PLAYERBOTS_MCMULTIPLIERS_H

#include "Multiplier.h"

// Several MC bosses spam raid-wide curses/magic debuffs faster than casual
// dispelling clears them (Lucifron's Doom+Curse, Shazzrah's +100% magic
// curse, Gehennas' -75% healing curse). While one is active, non-healer
// dispellers (mages, ret/prot paladins) treat cleansing as more important
// than their rotation. Healers are left alone so they keep triaging heals
// vs dispels normally.
class McDispelUrgencyMultiplier : public Multiplier
{
public:
    McDispelUrgencyMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "mc dispel urgency multiplier") {}
    float GetValue(Action* action) override;
};

class GarrDisableDpsAoeMultiplier : public Multiplier
{
public:
    GarrDisableDpsAoeMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "garr disable dps aoe multiplier") {}
    float GetValue(Action* action) override;
};

class BaronGeddonAbilityMultiplier : public Multiplier
{
public:
    BaronGeddonAbilityMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "baron geddon ability multiplier") {}
    float GetValue(Action* action) override;
};

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
