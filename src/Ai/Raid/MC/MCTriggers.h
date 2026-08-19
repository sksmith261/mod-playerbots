#ifndef PLAYERBOTS_MCTRIGGERS_H
#define PLAYERBOTS_MCTRIGGERS_H

#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Trigger.h"

// Generic marking/frenzy/fear/ground-effect triggers now live in
// src/Ai/Raid/RaidBossScripts.h; only MC-specific triggers remain here.

class McGarrBanishTrigger : public Trigger
{
public:
    McGarrBanishTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc garr banish") {}
    bool IsActive() override;
};

class McLivingBombDebuffTrigger : public Trigger
{
public:
    McLivingBombDebuffTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc living bomb debuff") {}
    bool IsActive() override;
};

class McBaronGeddonInfernoTrigger : public Trigger
{
public:
    McBaronGeddonInfernoTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc baron geddon inferno") {}
    bool IsActive() override;
};

class McShazzrahRangedTrigger : public Trigger
{
public:
    McShazzrahRangedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc shazzrah ranged") {}
    bool IsActive() override;
};

class McShazzrahPurgeTrigger : public Trigger
{
public:
    McShazzrahPurgeTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc shazzrah purge") {}
    bool IsActive() override;
};

class McGolemaggMarkBossTrigger : public Trigger
{
public:
    McGolemaggMarkBossTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc golemagg mark boss") {}
    bool IsActive() override;
};

class McGolemaggIsMainTankTrigger : public Trigger
{
public:
    McGolemaggIsMainTankTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc golemagg is main tank") {}
    bool IsActive() override;
};

class McGolemaggIsAssistTankTrigger : public Trigger
{
public:
    McGolemaggIsAssistTankTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc golemagg is assist tank") {}
    bool IsActive() override;
};

class McCoreHoundMarkTrigger : public Trigger
{
public:
    McCoreHoundMarkTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc core hound mark") {}
    bool IsActive() override;
};


// Baron Geddon is channeling Armageddon (sub-2% self-detonation): everyone
// within blast range runs, whatever their role.
class McArmageddonTrigger : public Trigger
{
public:
    McArmageddonTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc geddon armageddon") {}
    bool IsActive() override;
};

// Ragnaros wipes the raid with Magma Blast whenever his melee range stays
// empty for ~4s (knockbacks empty it constantly): tanks sprint back in.
class McRagnarosTankReentryTrigger : public Trigger
{
public:
    McRagnarosTankReentryTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc ragnaros tank reentry") {}
    bool IsActive() override;
};


// Ranged/healers hold the safe-platform camp during Ragnaros so Might of
// Ragnaros punts land on rock instead of lava.
class McRagnarosRangedCampTrigger : public Trigger
{
public:
    McRagnarosRangedCampTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc ragnaros ranged camp") {}
    bool IsActive() override;
};

// A Firesworn near this melee bot is about to die (Eruption on death).
class McGarrEruptionTrigger : public Trigger
{
public:
    McGarrEruptionTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc garr eruption") {}
    bool IsActive() override;
};

#endif
