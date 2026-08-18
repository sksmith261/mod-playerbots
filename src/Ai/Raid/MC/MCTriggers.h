#ifndef PLAYERBOTS_MCTRIGGERS_H
#define PLAYERBOTS_MCTRIGGERS_H

#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Trigger.h"

// Main tank keeps the skull mark maintained while the named boss is active;
// which unit gets skulled is the paired McKillOrderMarkAction's decision.
class McKillOrderMarkTrigger : public Trigger
{
public:
    McKillOrderMarkTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

class McGarrBanishTrigger : public Trigger
{
public:
    McGarrBanishTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc garr banish") {}
    bool IsActive() override;
};

class McGarrMarkTrigger : public Trigger
{
public:
    McGarrMarkTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc garr mark") {}
    bool IsActive() override;
};

class McMagmadarFrenzyTrigger : public Trigger
{
public:
    McMagmadarFrenzyTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc magmadar frenzy") {}
    bool IsActive() override;
};

class McMagmadarTremorTotemTrigger : public Trigger
{
public:
    McMagmadarTremorTotemTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc magmadar tremor totem") {}
    bool IsActive() override;
};

class McMagmadarFearWardTrigger : public Trigger
{
public:
    McMagmadarFearWardTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc magmadar fear ward") {}
    bool IsActive() override;
};

// Bot is standing in a damaging ground effect (identified by the periodic
// aura it applies) and should step out.
class McGroundEffectAuraTrigger : public Trigger
{
public:
    McGroundEffectAuraTrigger(PlayerbotAI* botAI, std::string const name, uint32 spellId)
        : Trigger(botAI, name), spellId(spellId) {}
    bool IsActive() override;

protected:
    uint32 const spellId;
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

// Ragnaros has submerged and Sons of Flame are up; the main tank drives the
// kill-order mark across them.
class McRagnarosSonsTrigger : public Trigger
{
public:
    McRagnarosSonsTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc ragnaros sons") {}
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

#endif
