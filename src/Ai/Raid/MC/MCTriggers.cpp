#include "MCTriggers.h"

#include "SharedDefines.h"
#include "MCActions.h"
#include "MCHelpers.h"

using namespace MoltenCoreHelpers;

bool McKillOrderMarkTrigger::IsActive()
{
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", bossName);
}

bool McGarrBanishTrigger::IsActive()
{
    if (bot->getClass() != CLASS_WARLOCK || !bot->HasSpell(SPELL_BANISH_R1))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "garr"))
        return false;

    Unit* assigned = GetGarrBanishAssignment(botAI, bot);
    return assigned && !IsBanished(assigned) && !assigned->HasAura(SPELL_SEPARATION_ANXIETY_MINION);
}

bool McGarrMarkTrigger::IsActive()
{
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", "garr");
}

bool McMagmadarFrenzyTrigger::IsActive()
{
    if (bot->getClass() != CLASS_HUNTER)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "magmadar");
    return boss && boss->HasAura(SPELL_MAGMADAR_FRENZY);
}

bool McMagmadarTremorTotemTrigger::IsActive()
{
    if (bot->getClass() != CLASS_SHAMAN || !bot->HasSpell(SPELL_TREMOR_TOTEM))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "magmadar"))
        return false;

    // Any shaman's totem in range covers the camp; don't stack duplicates.
    return !bot->FindNearestCreature(NPC_TREMOR_TOTEM, 20.0f);
}

bool McMagmadarFearWardTrigger::IsActive()
{
    if (bot->getClass() != CLASS_PRIEST || !bot->HasSpell(SPELL_FEAR_WARD))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "magmadar"))
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

bool McMagmadarLavaBombTrigger::IsActive()
{
    // No boss check: the fire patch (and its DoT) outlives target switches.
    return bot->HasAura(SPELL_LAVA_BOMB_DOT);
}

bool McLivingBombDebuffTrigger::IsActive()
{
    // No check for Baron Geddon, because bots may have the bomb even after Geddon died.
    return bot->HasAura(SPELL_LIVING_BOMB);
}

bool McBaronGeddonInfernoTrigger::IsActive()
{
    if (Unit* boss = AI_VALUE2(Unit*, "find target", "baron geddon"))
        return boss->HasAura(SPELL_INFERNO);
    return false;
}

bool McShazzrahRangedTrigger::IsActive()
{
    return AI_VALUE2(Unit*, "find target", "shazzrah") && PlayerbotAI::IsRanged(bot);
}

bool McShazzrahPurgeTrigger::IsActive()
{
    bool const canStrip = (bot->getClass() == CLASS_SHAMAN && bot->HasSpell(SPELL_PURGE_R1)) ||
                          (bot->getClass() == CLASS_PRIEST && bot->HasSpell(SPELL_DISPEL_MAGIC_R1));
    if (!canStrip)
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "shazzrah");
    return boss && boss->HasAura(SPELL_DEADEN_MAGIC);
}

bool McGolemaggMarkBossTrigger::IsActive()
{
    // any tank may mark the boss
    return AI_VALUE2(Unit*, "find target", "golemagg the incinerator") && PlayerbotAI::IsTank(bot);
}

bool McGolemaggIsMainTankTrigger::IsActive()
{
    return AI_VALUE2(Unit*, "find target", "golemagg the incinerator") && PlayerbotAI::IsMainTank(bot);
}

bool McGolemaggIsAssistTankTrigger::IsActive()
{
    return AI_VALUE2(Unit*, "find target", "golemagg the incinerator") && PlayerbotAI::IsAssistTank(bot);
}

bool McCoreHoundMarkTrigger::IsActive()
{
    return PlayerbotAI::IsMainTank(bot) && AI_VALUE2(Unit*, "find target", "core hound");
}
