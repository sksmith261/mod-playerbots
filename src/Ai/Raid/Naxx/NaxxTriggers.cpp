#include "NaxxTriggers.h"

#include "Playerbots.h"
#include "NaxxSpellIds.h"
#include "Timer.h"
#include "Trigger.h"
#include "NaxxBossHelper.h"

bool MutatingInjectionMeleeTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return false;

    return MutatingInjectionTrigger::IsActive() && !botAI->IsRanged(bot);
}

bool MutatingInjectionRangedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return false;

    return MutatingInjectionTrigger::IsActive() && botAI->IsRanged(bot);
}

bool AuraRemovedTrigger::IsActive()
{
    bool check = botAI->HasAura(name, bot, false, false, -1, true);
    bool ret = false;
    if (prev_check && !check)
        ret = true;

    prev_check = check;
    return ret;
}

bool MutatingInjectionRemovedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return false;

    return HasNoAuraTrigger::IsActive() && botAI->GetState() == BOT_STATE_COMBAT && botAI->IsRanged(bot);
}

bool GrobbulusCloudTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return false;

    if (!botAI->IsMainTank(bot))
        return false;

    // bot->Yell("has aggro on " + boss->GetName() + " : " + to_string(AI_VALUE2(bool, "has aggro", "boss target")),
    // LANG_UNIVERSAL);
    if (!AI_VALUE2(bool, "has aggro", "boss target"))
        return false;

    uint32 now = getMSTime();
    bool poison_cloud_casting = false;
    if (boss->HasUnitState(UNIT_STATE_CASTING))
    {
        Spell* spell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            spell = boss->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

        if (spell)
            poison_cloud_casting = NaxxSpellIds::MatchesAnySpellId(spell->GetSpellInfo(), {NaxxSpellIds::PoisonCloud});

    }
    if (!poison_cloud_casting && last_cloud_ms != 0 && now - last_cloud_ms < CloudRotationDelayMs)
        return false;

    last_cloud_ms = now;
    return true;
}

bool MaexxnaWebWrapTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "maexxna");
    if (!boss || !boss->IsAlive())
        return false;

    // Designated rescuers only: the first four assist ranged DPS.
    bool rescuer = false;
    for (uint8 i = 0; i < 4; ++i)
        if (botAI->IsAssistRangedDpsOfIndex(bot, i))
        {
            rescuer = true;
            break;
        }

    if (!rescuer)
        return false;

    // Grid search is legal here: gated on the boss being on our threat list.
    return bot->FindNearestCreature(NaxxHelpers::NPC_WEB_WRAP, 120.0f) != nullptr;
}

bool NothTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (Unit* boss = AI_VALUE2(Unit*, "find target", "noth the plaguebringer"))
        return boss->IsAlive();

    // Balcony phase: Noth is untargetable but his skeletons are on us.
    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && NaxxHelpers::IsNothAdd(botAI, unit))
            return true;
    }

    return false;
}

bool LoathebSporeTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "loatheb");
    if (!boss || !boss->IsAlive())
        return false;

    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return false;

    if (NaxxHelpers::HasSporeBuff(botAI, bot))
        return false;

    if (!NaxxHelpers::IsSporeSoaker(botAI, bot))
        return false;

    return NaxxHelpers::NearestLoathebSpore(botAI, bot) != nullptr;
}

bool GothikTrigger::IsActive()
{
    if (!bot->IsInCombat())
        return false;

    if (Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester"))
        return boss->IsAlive();

    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && NaxxHelpers::GothikAddRank(botAI, unit) > 0)
            return true;
    }

    return false;
}

bool AnubrekhanSpreadTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss || !boss->IsAlive())
        return false;

    if (!PlayerbotAI::IsRanged(bot) && !PlayerbotAI::IsHeal(bot))
        return false;

    // The swarm phase owns movement (MT kites the wall, raid gathers center).
    if (NaxxHelpers::AnubrekhanSwarmActive(botAI, boss))
        return false;

    float x, y;
    NaxxHelpers::AnubrekhanSpreadSlot(botAI, bot, x, y);
    return bot->GetExactDist2d(x, y) > 5.0f;
}

bool FaerlinaWorshipperDutyTrigger::IsActive()
{
    if (!botAI->IsAssistTank(bot))
        return false;

    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    return boss && boss->IsAlive();
}

bool FaerlinaFrenzyTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss || !boss->IsAlive())
        return false;

    return botAI->HasAura("frenzy", boss);
}

bool HeiganDanceTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    return boss && boss->IsAlive();
}

//bool HeiganMeleeTrigger::IsActive()
//{
//    Unit* heigan = AI_VALUE2(Unit*, "find target", "heigan the unclean");
//    if (!heigan)
//    {
//        return false;
//    }
//    return !botAI->IsRanged(bot);
//}
//
//bool HeiganRangedTrigger::IsActive()
//{
//    Unit* heigan = AI_VALUE2(Unit*, "find target", "heigan the unclean");
//    if (!heigan)
//    {
//        return false;
//    }
//    return botAI->IsRanged(bot);
//}

bool RazuviousTankTrigger::IsActive()
{
    Difficulty diff = bot->GetRaidDifficulty();
    if (diff == RAID_DIFFICULTY_10MAN_NORMAL)
        return helper.UpdateBossAI() && botAI->IsTank(bot);

    return helper.UpdateBossAI() && bot->getClass() == CLASS_PRIEST;
}

bool RazuviousNontankTrigger::IsActive()
{
    Difficulty diff = bot->GetRaidDifficulty();
    if (diff == RAID_DIFFICULTY_10MAN_NORMAL)
        return helper.UpdateBossAI() && !(botAI->IsTank(bot));

    return helper.UpdateBossAI() && !(bot->getClass() == CLASS_PRIEST);
}

bool FourHorsemenDutyTrigger::IsActive() { return helper.UpdateBossAI(); }

bool FourHorsemenAttractorsTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsAttracter(bot);
}

bool FourHorsemenExceptAttractorsTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return !helper.IsAttracter(bot);
}

bool SapphironGroundTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseGround();
}

bool SapphironFlightTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseFlight();
}

bool GluthTrigger::IsActive() { return helper.UpdateBossAI(); }

bool GluthMainTankMortalWoundTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    if (!botAI->IsAssistTankOfIndex(bot, 0))
        return false;

    Unit* mt = AI_VALUE(Unit*, "main tank");
    if (!mt)
        return false;

    Aura* aura = NaxxSpellIds::GetAnyAura(mt, {NaxxSpellIds::MortalWound10, NaxxSpellIds::MortalWound25});
    if (!aura)
    {
        // Fallback to name for custom spell data.
        aura = botAI->GetAura("mortal wound", mt, false, true);
    }
    if (!aura || aura->GetStackAmount() < 5)
        return false;

    return true;
}

bool KelthuzadTrigger::IsActive() { return helper.UpdateBossAI(); }

bool AnubrekhanTrigger::IsActive() {
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return false;

    return true;
}

bool FaerlinaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!boss)
        return false;

    return true;
}

bool MaexxnaTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "maexxna");
    if (!boss)
        return false;

    return !botAI->IsTank(bot);
}

bool PatchwerkTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    if (!boss)
    {
        return false;
    }
    return !botAI->IsTank(bot) && !botAI->IsRanged(bot);
}

bool PatchwerkRangedTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    if (!boss)
    {
        return false;
    }
    return !botAI->IsTank(bot) && botAI->IsRanged(bot);
}

bool PatchwerkNonTankTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "patchwerk");
    if (!boss)
    {
        return false;
    }
    return !botAI->IsTank(bot);
}

bool LoathebTrigger::IsActive() { return helper.UpdateBossAI(); }

bool ThaddiusPhasePetTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhasePet();
}

bool ThaddiusPhaseTransitionTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseTransition();
}

bool ThaddiusPhaseThaddiusTrigger::IsActive()
{
    if (!helper.UpdateBossAI())
        return false;

    return helper.IsPhaseThaddius();
}
