#ifndef PLAYERBOTS_MCTRIGGERS_H
#define PLAYERBOTS_MCTRIGGERS_H

#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "Trigger.h"

class McLucifronMarkTrigger : public Trigger
{
public:
    McLucifronMarkTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc lucifron mark") {}
    bool IsActive() override;
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

class McMagmadarLavaBombTrigger : public Trigger
{
public:
    McMagmadarLavaBombTrigger(PlayerbotAI* botAI) : Trigger(botAI, "mc magmadar lava bomb") {}
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
