/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_RAIDBOSSSCRIPTS_H
#define PLAYERBOTS_RAIDBOSSSCRIPTS_H

// Reusable raid-boss building blocks, promoted from the Molten Core AI so
// every raid can consume them as one-line registrations:
//   - kill-order skull marking over tiers of adds
//   - hunter tranq urgency the moment a boss frenzy is up
//   - shaman tremor totem / priest fear ward against boss fears
//   - stepping out of damaging ground effects
//   - dispel urgency while curse/debuff-spam bosses are active

#include <string>
#include <vector>

#include "Action.h"
#include "AttackAction.h"
#include "MovementActions.h"
#include "Multiplier.h"
#include "Trigger.h"

class PlayerbotAI;

// Any living member of the bot's group is in combat — the gate that keeps
// positioning behaviors from marching out-of-combat bots across an instance
// toward a fight their raid is not actually in.
bool IsRaidGroupInCombat(Player* bot);

// Main tank keeps the skull mark maintained while the named boss is active;
// which unit gets skulled is the paired RaidKillOrderMarkAction's decision.
class RaidKillOrderMarkTrigger : public Trigger
{
public:
    RaidKillOrderMarkTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

// Main-tank trigger for add phases where the boss is unfindable (submerged,
// stealthed): active while any unit of the given entry is alive.
class RaidAddsAliveMarkTrigger : public Trigger
{
public:
    RaidAddsAliveMarkTrigger(PlayerbotAI* botAI, std::string const name, std::vector<uint32> const addEntries)
        : Trigger(botAI, name, 1 * 1000), addEntries(addEntries) {}
    bool IsActive() override;

protected:
    std::vector<uint32> const addEntries;
};

// Adds-first kill order via the skull mark: tiers of add entries die in
// order (most-damaged first within a tier, sticky so the mark doesn't flap),
// then the boss. avoidAuras lists shield auras (e.g. Majordomo's
// reflections) the mark prefers — and actively switches — away from. An
// empty bossName runs without a boss gate or boss fallback (Ragnaros' Sons
// phase, where the submerged boss is stealthed and unfindable).
class RaidKillOrderMarkAction : public Action
{
public:
    RaidKillOrderMarkAction(PlayerbotAI* botAI, std::string const name, std::string const bossName,
                            std::vector<uint32> const addEntries, std::vector<uint32> const avoidAuras = {})
        : Action(botAI, name), bossName(bossName), addEntries(addEntries), avoidAuras(avoidAuras) {};
    Unit* GetTarget() override;
    bool Execute(Event event) override;

protected:
    bool IsClean(Unit* unit) const;

    std::string const bossName;
    std::vector<uint32> const addEntries;
    std::vector<uint32> const avoidAuras;
};

// Hunter: the named boss has its frenzy up — fire tranq at raid priority so
// it never queues behind the rotation.
class RaidFrenzyTranqTrigger : public Trigger
{
public:
    RaidFrenzyTranqTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName,
                           uint32 frenzySpellId)
        : Trigger(botAI, name), bossName(bossName), frenzySpellId(frenzySpellId) {}
    bool IsActive() override;

protected:
    std::string const bossName;
    uint32 const frenzySpellId;
};

// Shaman: drop Tremor Totem while engaged with the named fear boss (skipped
// if any shaman's totem is already within 20y).
class RaidTremorTotemTrigger : public Trigger
{
public:
    RaidTremorTotemTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name, 2 * 1000), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

// The named boss is attacking a non-tank (knockback threat drop, blink,
// teleport, fear): an elected taunt-capable bot tank takes it back. Fires
// only for the elected tank; every bot computes the same election.
class RaidBackupTauntTrigger : public Trigger
{
public:
    RaidBackupTauntTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

class RaidBackupTauntAction : public AttackAction
{
public:
    RaidBackupTauntAction(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : AttackAction(botAI, name), bossName(bossName) {}
    bool Execute(Event event) override;

protected:
    std::string const bossName;
};

// Tanks sprint back to the named boss after being punted out of melee range
// (wing buffets, knock-aways) — melee range going empty hands the boss to
// the raid or triggers punish mechanics.
class RaidTankReentryTrigger : public Trigger
{
public:
    RaidTankReentryTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName, float maxRange)
        : Trigger(botAI, name), bossName(bossName), maxRange(maxRange) {}
    bool IsActive() override;

protected:
    std::string const bossName;
    float const maxRange;
};

class RaidTankReentryAction : public MovementAction
{
public:
    RaidTankReentryAction(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : MovementAction(botAI, name), bossName(bossName) {}
    bool Execute(Event event) override;

protected:
    std::string const bossName;
};

// A damaging ground-hazard GameObject (lava burst rune, bomb patch) is
// within radius of the bot — the only shared step-out primitive keyed on
// the player AURA misses hazards that hurt on contact with no debuff.
class RaidNearGameObjectTrigger : public Trigger
{
public:
    RaidNearGameObjectTrigger(PlayerbotAI* botAI, std::string const name, uint32 goEntry, float radius)
        : Trigger(botAI, name), goEntry(goEntry), radius(radius) {}
    bool IsActive() override;

protected:
    uint32 const goEntry;
    float const radius;
};

class RaidMoveFromGameObjectAction : public MovementAction
{
public:
    RaidMoveFromGameObjectAction(PlayerbotAI* botAI, std::string const name, uint32 goEntry, float radius)
        : MovementAction(botAI, name), goEntry(goEntry), radius(radius) {}
    bool Execute(Event event) override;

protected:
    uint32 const goEntry;
    float const radius;
};

// Non-victims keep to the named boss's rear flank (the base "rear flank"
// action's 90-120 degree band) — one registration per boss with a frontal
// cone/cleave/breath and/or tail sweep. The victim (tank) is exempt.
class RaidRearFlankTrigger : public Trigger
{
public:
    RaidRearFlankTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

// Ranged and healers hold a minimum range band from the named boss —
// closest-N target filters (Huhuran), big point-blank AoEs (Skeram, Blast
// Wave bosses), and PBAoE fears all read as "stand at least this far out".
// Tanks, melee, and the boss's current victim are exempt.
class RaidStandoffTrigger : public Trigger
{
public:
    RaidStandoffTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName, float minRange,
                        bool includeHealers = true)
        : Trigger(botAI, name), bossName(bossName), minRange(minRange), includeHealers(includeHealers) {}
    bool IsActive() override;

protected:
    std::string const bossName;
    float const minRange;
    bool const includeHealers;
};

class RaidStandoffAction : public MovementAction
{
public:
    RaidStandoffAction(PlayerbotAI* botAI, std::string const name, std::string const bossName, float minRange)
        : MovementAction(botAI, name), bossName(bossName), minRange(minRange) {}
    bool Execute(Event event) override;

protected:
    std::string const bossName;
    float const minRange;
};

// Priest: keep Fear Ward on the main tank while the named fear boss is
// active.
class RaidFearWardTrigger : public Trigger
{
public:
    RaidFearWardTrigger(PlayerbotAI* botAI, std::string const name, std::string const bossName)
        : Trigger(botAI, name), bossName(bossName) {}
    bool IsActive() override;

protected:
    std::string const bossName;
};

class RaidFearWardAction : public Action
{
public:
    RaidFearWardAction(PlayerbotAI* botAI, std::string const name) : Action(botAI, name) {};
    bool Execute(Event event) override;
};

// Bot is standing in a damaging ground effect (identified by the periodic
// aura it applies) and should step out.
class RaidGroundEffectAuraTrigger : public Trigger
{
public:
    RaidGroundEffectAuraTrigger(PlayerbotAI* botAI, std::string const name, uint32 spellId)
        : Trigger(botAI, name), spellId(spellId) {}
    bool IsActive() override;

protected:
    uint32 const spellId;
};

class RaidMoveFromGroundEffectAction : public MovementAction
{
public:
    RaidMoveFromGroundEffectAction(PlayerbotAI* botAI, std::string const name)
        : MovementAction(botAI, name) {};
    bool Execute(Event event) override;
};

// While one of the listed bosses is active, non-healer dispellers (mages,
// ret/prot paladins) treat cleansing party members as more important than
// their rotation. Healers are left alone so they keep triaging heals vs
// dispels normally.
class RaidDispelUrgencyMultiplier : public Multiplier
{
public:
    RaidDispelUrgencyMultiplier(PlayerbotAI* botAI, std::string const name,
                                std::vector<std::string> const bossNames)
        : Multiplier(botAI, name), bossNames(bossNames) {}
    float GetValue(Action* action) override;

protected:
    std::vector<std::string> const bossNames;
};

#endif
