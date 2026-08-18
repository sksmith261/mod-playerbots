/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "RaidBossScripts.h"

#include "Event.h"
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
}

bool RaidKillOrderMarkTrigger::IsActive()
{
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", bossName);
}

bool RaidAddsAliveMarkTrigger::IsActive()
{
    if (!PlayerbotAI::IsMainTank(bot))
        return false;

    for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
    {
        Unit* unit = botAI->GetUnit(target);
        if (unit && unit->IsAlive() && unit->GetEntry() == addEntry)
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

    for (uint32 entry : addEntries)
    {
        Unit* best = nullptr;
        bool bestClean = false;
        for (auto const& target : AI_VALUE(GuidVector, "possible targets no los"))
        {
            Unit* unit = botAI->GetUnit(target);
            if (!unit || !unit->IsAlive() || unit->GetEntry() != entry)
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

    // Any shaman's totem in range covers the camp; don't stack duplicates.
    return !bot->FindNearestCreature(NPC_TREMOR_TOTEM, 20.0f);
}

bool RaidFearWardTrigger::IsActive()
{
    if (bot->getClass() != CLASS_PRIEST || !bot->HasSpell(SPELL_FEAR_WARD))
        return false;

    if (!AI_VALUE2(Unit*, "find target", bossName))
        return false;

    if (Group* group = bot->GetGroup())
        for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
        {
            Player* member = itr->GetSource();
            if (member && member->IsAlive() && botAI->IsMainTank(member))
                return !member->HasAura(SPELL_FEAR_WARD);
        }

    return false;
}

bool RaidFearWardAction::Execute(Event /*event*/)
{
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    for (GroupReference* itr = group->GetFirstMember(); itr != nullptr; itr = itr->next())
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive() && botAI->IsMainTank(member))
        {
            if (member->HasAura(SPELL_FEAR_WARD))
                return false;

            return botAI->CastSpell(SPELL_FEAR_WARD, member);
        }
    }

    return false;
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
