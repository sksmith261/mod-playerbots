#include "NaxxTriggers.h"
#include "RaidDirector.h"

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

    // Deadline path. The 40-man script casts Poison Cloud as a TRIGGERED
    // spell — instant, invisible to GetCurrentSpell — so the cast detection
    // below has never once fired there, and the kite advanced on a blind 15s
    // timer whose phase against the real drops was pure luck. In phase, the
    // tank stepped right after each drop and the fight looked solved; out of
    // phase he stood in every fresh cloud for up to 15 seconds. The plan now
    // carries the script's own schedule: step the moment a drop lands (the
    // cloud is born small, at the spot he is leaving). "Just re-armed" also
    // counts as a drop, so a director rebuild slipping between our ticks
    // cannot swallow the step — and it fires on the pull, which starts the
    // rotation before the first cloud instead of after it.
    if (RaidPlan const* plan = RaidDirector::Get(bot))
        if (plan->nextEventKind == RAID_EVENT_GROBBULUS_CLOUD && plan->nextEventMs)
        {
            uint32 const deadline = plan->nextEventMs;
            bool const dropped = now >= deadline || deadline - now > 12500;
            if (dropped && (!last_cloud_ms || now - last_cloud_ms > 5000))
            {
                last_cloud_ms = now;
                return true;
            }

            return false;
        }

    // Legacy fallback: no plan or an IP build that does not publish the
    // clock. Kept byte-for-byte so behaviour without the data is unchanged.
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
    return !NaxxHelpers::MaexxnaWebWraps(botAI, bot).empty();
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

    // Grid-backed: Gothik holds threat on almost nobody during the wave
    // phase, so a threat-only lookup left most of the raid outside the
    // encounter AI entirely.
    if (Unit* boss = NaxxHelpers::FindGothik(botAI, bot))
        if (boss->IsAlive())
            return true;

    for (auto const& guid : AI_VALUE(GuidVector, "attackers"))
    {
        Unit* unit = botAI->GetUnit(guid);
        if (unit && unit->IsAlive() && NaxxHelpers::GothikAddRank(botAI, unit) > 0)
            return true;
    }

    return false;
}

bool GrobbulusRangedPositionTrigger::IsActive()
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss || !boss->IsAlive())
        return false;

    return PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot);
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
    // Reizan runs IP's naxx-40: the vanilla mechanic (priests mind control
    // the Understudies; obedience crystals do not exist there). The raid's
    // nominal difficulty says nothing about which version this is, so the
    // priest path is unconditional — the wotlk 10-man crystal path was
    // unreachable-by-design here and its grid/spawn-id code was broken
    // anyway (audit).
    return helper.UpdateBossAI() && bot->getClass() == CLASS_PRIEST;
}

bool RazuviousNontankTrigger::IsActive()
{
    return helper.UpdateBossAI() && !(bot->getClass() == CLASS_PRIEST);
}

bool FourHorsemenDutyTrigger::IsActive()
{
    RaidPlan const* plan = RaidDirector::Get(bot);
    return plan && plan->encounter == RAID_ENCOUNTER_FOUR_HORSEMEN && plan->For(bot->GetGUID());
}

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
    // Reads the plan rather than this bot's threat list. Sapphiron holds
    // threat on almost nobody, so the old check left most of the raid with
    // no Sapphiron behaviour at all.
    RaidPlan const* plan = RaidDirector::Get(bot);
    return plan && plan->encounter == RAID_ENCOUNTER_SAPPHIRON && plan->For(bot->GetGUID());
}

bool SapphironFlightTrigger::IsActive()
{
    RaidPlan const* plan = RaidDirector::Get(bot);
    return plan && plan->encounter == RAID_ENCOUNTER_SAPPHIRON && plan->For(bot->GetGUID());
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

bool ThaddiusTetherTrigger::IsActive()
{
    if (!helper.UpdateBossAI() || !helper.IsPhasePet())
        return false;

    Unit* pet = helper.GetHeldPet(bot);
    if (!pet)
        return false;

    Creature* creature = pet->ToCreature();
    if (!creature)
        return false;

    // 12y of the 28y budget: correct early, never race the break.
    Position const& home = creature->GetHomePosition();
    return creature->GetExactDist2d(home.GetPositionX(), home.GetPositionY()) > 12.0f;
}

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
