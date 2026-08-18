#ifndef PLAYERBOTS_AQ40TRIGGERS_H
#define PLAYERBOTS_AQ40TRIGGERS_H

#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RaidBossScripts.h"
#include "Trigger.h"

class Aq40InStomachTrigger : public Trigger
{
public:
    Aq40InStomachTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 in stomach") {}
    bool IsActive() override;
};

// The Eye of C'Thun is sweeping Dark Glare and the beam is closing on this
// bot's angular position.
class Aq40DarkGlareTrigger : public Trigger
{
public:
    Aq40DarkGlareTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 dark glare") {}
    bool IsActive() override;
};

// Mark-owner gate via the shared trigger; the paired action skulls the one
// Skeram that is not a summoned image.
class Aq40SkeramMarkTrigger : public RaidKillOrderMarkTrigger
{
public:
    Aq40SkeramMarkTrigger(PlayerbotAI* botAI)
        : RaidKillOrderMarkTrigger(botAI, "aq40 skeram mark", "the prophet skeram") {}
};

// Something next to this bot is whirlwinding (Sartura or a Royal Guard).
class Aq40SarturaWhirlwindTrigger : public Trigger
{
public:
    Aq40SarturaWhirlwindTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 sartura whirlwind") {}
    bool IsActive() override;
};

// Tank with no twin targeted while the twins are up (post-teleport pickup).
class Aq40TwinsTankPickupTrigger : public Trigger
{
public:
    Aq40TwinsTankPickupTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 twins tank pickup") {}
    bool IsActive() override;
};

// This bot is Vek'lor's victim and the twins are close enough to heal
// each other.
class Aq40TwinsSeparateTrigger : public Trigger
{
public:
    Aq40TwinsSeparateTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 twins separate") {}
    bool IsActive() override;
};

// Caster DPS standing in Vek'lor's Arcane Burst range.
class Aq40TwinsCasterRangeTrigger : public Trigger
{
public:
    Aq40TwinsCasterRangeTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 twins caster range") {}
    bool IsActive() override;
};

// An Ouro dirt mound is chasing this bot (they fixate with massive threat
// and quake ~10y around themselves): keep moving.
class Aq40OuroMoundTrigger : public Trigger
{
public:
    Aq40OuroMoundTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 ouro mound") {}
    bool IsActive() override;
};

// DPS bot is attacking the twin its damage type cannot hurt.
class Aq40TwinsWrongTargetTrigger : public Trigger
{
public:
    Aq40TwinsWrongTargetTrigger(PlayerbotAI* botAI) : Trigger(botAI, "aq40 twins wrong target") {}
    bool IsActive() override;
};
#endif
