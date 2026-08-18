#ifndef PLAYERBOTS_MCMULTIPLIERS_H
#define PLAYERBOTS_MCMULTIPLIERS_H

#include "Multiplier.h"

// Lucifron spams the whole raid with Impending Doom (magic) and Lucifron's
// Curse; non-healer dispellers (mages, ret/prot paladins) should treat
// cleansing as more important than their rotation while he is up. Healers are
// left alone so they keep triaging heals vs dispels normally.
class LucifronDispelMultiplier : public Multiplier
{
public:
    LucifronDispelMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "lucifron dispel multiplier") {}
    float GetValue(Action* action) override;
};

// Shazzrah's Curse doubles magic damage taken right as he spams Arcane
// Explosion — same shape as Lucifron: non-healer dispellers prioritize
// decursing over rotation. (Kept as a separate multiplier per boss to avoid
// cross-session merge conflicts in shared files.)
class ShazzrahDispelMultiplier : public Multiplier
{
public:
    ShazzrahDispelMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "shazzrah dispel multiplier") {}
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

class GolemaggMultiplier : public Multiplier
{
public:
    GolemaggMultiplier(PlayerbotAI* botAI) : Multiplier(botAI, "golemagg multiplier") {}
    float GetValue(Action* action) override;
};

#endif
