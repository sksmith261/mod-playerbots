#include "MCTriggers.h"

#include "SharedDefines.h"
#include "MCActions.h"
#include "MCHelpers.h"

using namespace MoltenCoreHelpers;

bool McGarrBanishTrigger::IsActive()
{
    if (bot->getClass() != CLASS_WARLOCK || !bot->HasSpell(SPELL_BANISH_R1))
        return false;

    if (!AI_VALUE2(Unit*, "find target", "garr"))
        return false;

    Unit* assigned = GetGarrBanishAssignment(botAI, bot);
    return assigned && !IsBanished(assigned) && !assigned->HasAura(SPELL_SEPARATION_ANXIETY_MINION);
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

bool McArmageddonTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "baron geddon");
    return boss && boss->HasAura(SPELL_ARMAGEDDON) && bot->GetDistance(boss) < 35.0f;
}

bool McRagnarosTankReentryTrigger::IsActive()
{
    if (!PlayerbotAI::IsTank(bot) || !bot->IsAlive())
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "ragnaros");
    return boss && boss->IsAlive() && bot->GetDistance(boss) > 8.0f;
}
