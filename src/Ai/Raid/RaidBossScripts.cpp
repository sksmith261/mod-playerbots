/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "RaidBossScripts.h"

#include "Event.h"
#include "GameObject.h"
#include "GenericSpellActions.h"
#include "Group.h"
#include "Playerbots.h"
#include "RtiTargetValue.h"
#include "SharedDefines.h"

namespace
{
constexpr uint32 NPC_TREMOR_TOTEM = 5913;
constexpr uint32 SPELL_TREMOR_TOTEM = 8143;
constexpr uint32 SPELL_FEAR_WARD = 6346;

Player* FindMainTank(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && PlayerbotAI::IsMainTank(member))
            return member;
    }

    return nullptr;
}

// The bot responsible for maintaining raid marks. Normally the main tank —
// but when a human is main-tanking (no bot AI to run the trigger), marking
// must not go silently inert: the first living bot in shared group order
// takes over. Every bot computes the same answer, so exactly one marks.
bool IsMarkOwner(Player* bot)
{
    if (PlayerbotAI::IsMainTank(bot))
        return true;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    Player* mainTank = FindMainTank(bot);
    if (mainTank && GET_PLAYERBOT_AI(mainTank))
        return false;  // a bot main tank owns marking, and it is not us

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (!member || !member->IsAlive() || !GET_PLAYERBOT_AI(member))
            continue;

        return member == bot;
    }

    return false;
}
}

bool IsRaidGroupInCombat(Player* bot)
{
    Group* group = bot->GetGroup();
    if (!group)
        return bot->IsInCombat();

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && member->IsInCombat())
            return true;
    }

    return false;
}

bool RaidKillOrderMarkTrigger::IsActive()
{
    return IsMarkOwner(bot) && AI_VALUE2(Unit*, "find target", bossName);
}

bool RaidAddsAliveMarkTrigger::IsActive()
{
    if (!IsMarkOwner(bot))
        return false;

    // The grid scan below sees through walls at sight range: without combat
    // gates, standing bosses detected from the hallway got skull-marked and
    // every DPS bot charged the mark through live trash (Bug Trio report).
    // Mid-fight add phases — this trigger's real purpose — pass trivially.
    if (!IsRaidGroupInCombat(bot))
        return false;

    for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
    {
        // Creature guids encode the entry: filter before the ObjectAccessor
        // lookup so non-matching nearby units cost nothing.
        bool wanted = false;
        for (uint32 entry : addEntries)
            if (target.GetEntry() == entry)
            {
                wanted = true;
                break;
            }

        if (!wanted)
            continue;

        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->IsAlive() && unit->IsInCombat())
            return true;
    }

    return false;
}

bool RaidKillOrderMarkAction::IsClean(Unit* unit) const
{
    for (uint32 aura : avoidAuras)
        if (unit->HasAura(aura))
            return false;

    return true;
}

Unit* RaidKillOrderMarkAction::GetTarget()
{
    Unit* boss = nullptr;
    if (!bossName.empty())
    {
        boss = AI_VALUE2(Unit*, "find target", bossName);
        if (!boss)
            return nullptr;
    }

    Group* group = bot->GetGroup();
    if (!group)
        return nullptr;

    ObjectGuid currentSkullGuid = group->GetTargetIcon(RtiTargetValue::skullIndex);
    Unit* currentSkullUnit = currentSkullGuid.IsEmpty() ? nullptr : botAI->GetUnit(currentSkullGuid);

    // One grid search for all tiers: this value recalculates (a full grid
    // visit) on every Get(), so fetching it per tier multiplied the most
    // expensive lookup in the toolkit by the tier count.
    GuidVector const possibleTargets = AI_VALUE(GuidVector, "possible targets no los");

    for (uint32 entry : addEntries)
    {
        Unit* best = nullptr;
        bool bestClean = false;
        for (auto const& target : possibleTargets)
        {
            if (target.GetEntry() != entry)
                continue;

            Unit* unit = botAI->GetUnit(target);
            if (!unit || !unit->IsAlive() || !unit->IsInCombat())
                continue;

            // Prefer targets without a shield aura; the most damaged
            // otherwise, so an in-progress kill finishes.
            bool const clean = IsClean(unit);
            if (!best || (clean && !bestClean) ||
                (clean == bestClean && unit->GetHealth() < best->GetHealth()))
            {
                best = unit;
                bestClean = clean;
            }
        }

        if (!best)
            continue;  // tier cleared; next tier

        // Sticky within the active tier so the mark doesn't flap between
        // equally-valid adds — but switch off a target that picked up a
        // shield aura while a clean one exists.
        if (currentSkullUnit && currentSkullUnit->IsAlive() && currentSkullUnit->GetEntry() == entry)
        {
            if (IsClean(currentSkullUnit) || !bestClean)
                return nullptr;
        }

        return best;
    }

    // All add tiers cleared: skull the boss.
    if (boss && (currentSkullGuid.IsEmpty() || currentSkullGuid != boss->GetGUID()))
        return boss;

    return nullptr;
}

bool RaidKillOrderMarkAction::Execute(Event /*event*/)
{
    Unit* target = GetTarget();
    if (!target)
        return false;

    bot->GetGroup()->SetTargetIcon(RtiTargetValue::skullIndex, bot->GetGUID(), target->GetGUID());
    return true;
}

bool RaidFrenzyTranqTrigger::IsActive()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    return boss && boss->HasAura(frenzySpellId);
}

bool RaidTremorTotemTrigger::IsActive()
{
    if (bot->getClass() != CLASS_SHAMAN || !bot->HasSpell(SPELL_TREMOR_TOTEM))
        return false;

    if (!AI_VALUE2(Unit*, "find target", bossName))
        return false;

    // The totem must cover the melee camp the fear actually hits, not the
    // shaman's own feet: check coverage around the main tank, and only drop
    // if this shaman is close enough for its totem to reach the camp — a
    // ranged shaman's totem 30y out would satisfy a self-check while the
    // tank stays uncovered.
    Player* mainTank = FindMainTank(bot);
    if (!mainTank || bot->GetDistance(mainTank) > 20.0f)
        return false;

    return !mainTank->FindNearestCreature(NPC_TREMOR_TOTEM, 20.0f);
}

bool RaidStandoffTrigger::IsActive()
{
    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsMelee(bot))
        return false;

    if (!includeHealers && PlayerbotAI::IsHeal(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss || boss->GetVictim() == bot)
        return false;

    return bot->GetDistance(boss) < minRange;
}

bool RaidStandoffAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss)
        return false;

    // A little past the line so drift doesn't re-trigger every tick.
    return MoveAway(boss, minRange + 3.0f - bot->GetDistance(boss));
}

namespace
{
// A tank the backup election may pick: bot-controlled and of a class whose
// taunt the action can execute ("taunt spell" is aliased only by the
// warrior/paladin/DK tank strategies; druids use "growl").
bool CanExecuteBackupTaunt(Player* member)
{
    if (!GET_PLAYERBOT_AI(member))
        return false;

    switch (member->getClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
        case CLASS_DRUID:
            return true;
        default:
            return false;
    }
}
}

bool RaidBackupTauntTrigger::IsActive()
{
    if (!PlayerbotAI::IsTank(bot) || !bot->IsAlive())
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss || !boss->IsAlive() || !boss->IsInCombat())
        return false;

    Unit* victim = boss->GetVictim();
    if (victim)
        if (Player* victimPlayer = victim->ToPlayer())
            if (PlayerbotAI::IsTank(victimPlayer))
                return false;  // a tank already has it

    // Elected taker: first living taunt-capable bot tank in shared group
    // order that isn't the boss's current victim.
    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (!member || !member->IsAlive() || !PlayerbotAI::IsTank(member))
                continue;

            if (member == victim || !CanExecuteBackupTaunt(member))
                continue;

            return member == bot;
        }

    return false;
}

bool RaidBackupTauntAction::Execute(Event event)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss)
        return false;

    if (bot->GetVictim() != boss)
        return Attack(boss);

    std::string const tauntAction = bot->getClass() == CLASS_DRUID ? "growl" : "taunt spell";
    return botAI->DoSpecificAction(tauntAction, event, true);
}

bool RaidTankReentryTrigger::IsActive()
{
    if (!PlayerbotAI::IsTank(bot) || !bot->IsAlive())
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    return boss && boss->IsAlive() && boss->IsInCombat() && bot->GetDistance(boss) > maxRange;
}

bool RaidTankReentryAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", bossName);
    if (!boss)
        return false;

    return MoveNear(boss, 3.0f, MovementPriority::MOVEMENT_COMBAT);
}

bool RaidNearGameObjectTrigger::IsActive()
{
    return bot->IsAlive() && bot->FindNearestGameObject(goEntry, radius);
}

bool RaidMoveFromGameObjectAction::Execute(Event /*event*/)
{
    GameObject* hazard = bot->FindNearestGameObject(goEntry, radius);
    if (!hazard)
        return false;

    return FleePosition(hazard->GetPosition(), radius + 4.0f);
}

bool RaidRearFlankTrigger::IsActive()
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", bossName))
        return boss->GetVictim() != bot;

    return false;
}

bool RaidFearWardTrigger::IsActive()
{
    if (bot->getClass() != CLASS_PRIEST || !bot->HasSpell(SPELL_FEAR_WARD))
        return false;

    if (!AI_VALUE2(Unit*, "find target", bossName))
        return false;

    Player* mainTank = FindMainTank(bot);
    return mainTank && !mainTank->HasAura(SPELL_FEAR_WARD);
}

bool RaidFearWardAction::Execute(Event /*event*/)
{
    Player* mainTank = FindMainTank(bot);
    if (!mainTank || mainTank->HasAura(SPELL_FEAR_WARD))
        return false;

    return botAI->CastSpell(SPELL_FEAR_WARD, mainTank);
}

bool RaidGroundEffectAuraTrigger::IsActive()
{
    // No boss check: fire patches (and their DoTs) outlive target switches.
    return bot->HasAura(spellId);
}

bool RaidMoveFromGroundEffectAction::Execute(Event /*event*/)
{
    // Any direction out of the patch works; FleePosition picks a safe nearby
    // spot away from where the bot is standing.
    return FleePosition(bot->GetPosition(), 8.0f);
}

float RaidDispelUrgencyMultiplier::GetValue(Action* action)
{
    if (PlayerbotAI::IsHeal(bot) || !dynamic_cast<CurePartyMemberAction*>(action))
        return 1.0f;

    for (std::string const& boss : bossNames)
        if (AI_VALUE2(Unit*, "find target", boss))
            return 2.0f;

    return 1.0f;
}
