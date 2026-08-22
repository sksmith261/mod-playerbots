#include "NaxxMultipliers.h"

#include "ChooseTargetActions.h"
#include "DKActions.h"
#include "DruidActions.h"
#include "DruidBearActions.h"
#include "FollowActions.h"
#include "GenericActions.h"
#include "GenericSpellActions.h"
#include "HunterActions.h"
#include "MageActions.h"
#include "MovementActions.h"
#include "PaladinActions.h"
#include "PriestActions.h"
#include "NaxxActions.h"
#include "NaxxSpellIds.h"
#include "ReachTargetActions.h"
#include "RogueActions.h"
#include "ScriptedCreature.h"
#include "ShamanActions.h"
#include "Spell.h"
#include "UseMeetingStoneAction.h"
#include "WarriorActions.h"
#include "NaxxBossHelper.h"

float GothikBalconyMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    Unit* gothik = NaxxHelpers::FindGothik(botAI, bot);
    if (!gothik || !gothik->IsAlive())
        return 1.0f;

    if (action->GetTarget() != gothik)
        return 1.0f;

    // Wave phase: he sits passive on his balcony while the raid is meant to
    // be killing waves. Damage on him there is not free — it is DPS not
    // spent on the adds that actually end the attempt.
    if (NaxxHelpers::GothikWavePhase(gothik))
        return 0.0f;

    // Phase two: he teleports across the gate every 20s and wipes threat on
    // the far side. Until the gate opens at 30%, the half of the raid he
    // left cannot reach him — chasing means running at a shut gate.
    if (!NaxxHelpers::GothikGateOpen(gothik) &&
        NaxxHelpers::GothikLiveSide(gothik) != NaxxHelpers::GothikLiveSide(bot))
        return 0.0f;

    return 1.0f;
}

float FaerlinaDisciplineMultiplier::GetValue(Action* action)
{
    if (!action)
        return 1.0f;

    if (PlayerbotAI::IsTank(bot) || PlayerbotAI::IsHeal(bot))
        return 1.0f;

    Unit* faerlina = AI_VALUE2(Unit*, "find target", "grand widow faerlina");
    if (!faerlina || !faerlina->IsInCombat())
        return 1.0f;

    Unit* target = action->GetTarget();
    if (!target)
        return 1.0f;

    bool const frenzied = botAI->HasAura("frenzy", faerlina);

    if (!frenzied && botAI->EqualLowercaseName(target->GetName(), "naxxramas worshipper"))
        return 0.0f;

    if (frenzied && target == faerlina && NaxxHelpers::FaerlinaSacrificeTarget(botAI, faerlina))
        return 0.0f;

    return 1.0f;
}

float GrobbulusMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return 1.0f;

    if (dynamic_cast<AvoidAoeAction*>(action))
        return botAI->IsMainTank(bot) ? 0.0f : 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;

    return 1.0f;
}

//float HeiganDanceMultiplier::GetValue(Action* action)
//{
//    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
//    if (!boss)
//    {
//        return 1.0f;
//    }
//    bool platform_phase = boss->IsWithinDist2d(2794.26f, -3706.67f, 10.0f);
//    bool eruption_casting = false;
//    if (boss->HasUnitState(UNIT_STATE_CASTING))
//    {
//        Spell* spell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
//        if (!spell)
//        {
//            spell = boss->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
//        }
//        if (spell)
//        {
//            SpellInfo const* info = spell->GetSpellInfo();
//            bool isEruption = NaxxSpellIds::MatchesAnySpellId(info, {NaxxSpellIds::Eruption10});
//            if (!isEruption && info && info->SpellName[LOCALE_enUS])
//            {
//                // Fallback to name for custom spell data.
//                isEruption = botAI->EqualLowercaseName(info->SpellName[LOCALE_enUS], "eruption");
//            }
//            if (isEruption)
//            {
//                eruption_casting = true;
//            }
//        }
//    }
//    if (dynamic_cast<CombatFormationMoveAction*>(action) ||
//        dynamic_cast<CastDisengageAction*>(action) ||
//        dynamic_cast<CastBlinkBackAction*>(action) )
//    {
//        return 0.0f;
//    }
//    if (!platform_phase && !eruption_casting)
//    {
//        return 1.0f;
//    }
//    if (dynamic_cast<HeiganDanceAction*>(action) || dynamic_cast<CurePartyMemberAction*>(action))
//    {
//        return 1.0f;
//    }
//    if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastMeleeSpellAction*>(action))
//    {
//        CastSpellAction* spellAction = dynamic_cast<CastSpellAction*>(action);
//        uint32 spellId = AI_VALUE2(uint32, "spell id", spellAction->getSpell());
//        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
//        if (!spellInfo)
//        {
//            return 0.0f;
//        }
//        uint32 castTime = spellInfo->CalcCastTime();
//        if (castTime == 0 && !spellInfo->IsChanneled())
//        {
//            return 1.0f;
//        }
//    }
//    return 0.0f;
//}

float LoathebGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "loatheb");
    if (!boss)
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<FleeAction*>(action) ||
         dynamic_cast<CombatFormationMoveAction*>(action)))
    {
        return 0.0f;
    }
    if (!dynamic_cast<CastHealingSpellAction*>(action))
        return 1.0f;

    // Naxx-40 has no Necrotic Aura: Loatheb casts Corrupted Mind (29201),
    // which triggers one of four class-specific healing blocks. Keying on
    // the wotlk aura alone left this permanently disabled on that version.
    Aura* aura = NaxxSpellIds::GetAnyAura(
        bot, {NaxxSpellIds::NecroticAura10, NaxxSpellIds::CorruptedMindBlockA,
              NaxxSpellIds::CorruptedMindBlockB, NaxxSpellIds::CorruptedMindBlockC,
              NaxxSpellIds::CorruptedMindBlockD});
    if (!aura)
    {
        // Fallback to name for custom spell data.
        aura = botAI->GetAura("necrotic aura", bot);
    }
    if (!aura)
    {
        aura = botAI->GetAura("corrupted mind", bot);
    }
    if (!aura || aura->GetDuration() <= 1500)
        return 1.0f;

    return 0.0f;
}

float ThaddiusGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;
    // pet phase
    // A bot holding a pet must never flee with it — that is the one way to
    // drag it past its coil tether. The leash action (ACTION_RAID+3) always
    // leaves it a legal move, so nothing can freeze here.
    if (helper.IsPhasePet() && helper.GetHeldPet(bot) && dynamic_cast<FleeAction*>(action))
        return 0.0f;

    // ReachPartyMemberToHeal deliberately NOT zeroed here (upstream did):
    // with the raid split across two platforms, a healer whose lowest-health
    // target stands on the far side was blocked from ever closing to heal
    // range and simply stood there. Crossing is safe in the pet phase —
    // there are no marks or polarity to violate.
    if (helper.IsPhasePet() &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<BuffOnMainTankAction*>(action)))
    {
        return 0.0f;
    }
    // die at the same time
    Unit* target = AI_VALUE(Unit*, "current target");
    Unit* feugen = AI_VALUE2(Unit*, "find target", "feugen");
    Unit* stalagg = AI_VALUE2(Unit*, "find target", "stalagg");
    if (helper.IsPhasePet() && target && feugen && stalagg && target->GetHealthPct() <= 40 &&
        (feugen->GetHealthPct() >= target->GetHealthPct() + 3 || stalagg->GetHealthPct() >= target->GetHealthPct() + 3))
    {
        if (dynamic_cast<CastSpellAction*>(action) && !dynamic_cast<CastHealingSpellAction*>(action))
            return 0.0f;
    }
    // magnetic pull
    // uint32 curr_timer = eventMap->GetTimer();
    // // if (curr_phase == 2 && bot->GetPositionZ() > 312.5f && dynamic_cast<MovementAction*>(action))
    // {
    // if (curr_phase == 2 && (curr_timer % 20000 >= 18000 || curr_timer % 20000 <= 2000) &&
    // dynamic_cast<MovementAction*>(action))
    // {
    //     // MotionMaster *mm = bot->GetMotionMaster();
    //     // mm->Clear();
    //     return 0.0f;
    // }
    // thaddius phase
    // if (curr_phase == 8 && dynamic_cast<FleeAction*>(action))
    // {
    //         return 0.0f;
    // }
    return 1.0f;
}

float SapphironGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if (dynamic_cast<CastDeathGripAction*>(action) || dynamic_cast<CombatFormationMoveAction*>(action))
        return 0.0f;

    return 1.0f;
}

float InstructorRazuviousGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    context->GetValue<bool>("neglect threat")->Set(true);
    if (botAI->GetState() == BOT_STATE_COMBAT &&
        (dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
         dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action)))
    {
        return 0.0f;
    }
    return 1.0f;
}

float KelthuzadGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) || dynamic_cast<FleeAction*>(action)))
    {
        return 0.0f;
    }
    if (helper.IsPhaseOne())
    {
        if (dynamic_cast<CastTotemAction*>(action) || dynamic_cast<CastShadowfiendAction*>(action) ||
            dynamic_cast<CastRaiseDeadAction*>(action) || dynamic_cast<CastFeignDeathAction*>(action) ||
            dynamic_cast<CastInvisibilityAction*>(action) || dynamic_cast<CastVanishAction*>(action) ||
            dynamic_cast<PetAttackAction*>(action))
        {
            return 0.0f;
        }
    }
    if (helper.IsPhaseTwo())
    {
        if (dynamic_cast<CastBlizzardAction*>(action) || dynamic_cast<CastFrostNovaAction*>(action))
            return 0.0f;

    }
    return 1.0f;
}

float AnubrekhanGenericMultiplier::GetValue(Action* action)
{
    Unit* boss = AI_VALUE2(Unit*, "find target", "anub'rekhan");
    if (!boss)
        return 1.0f;

    if (NaxxSpellIds::HasAnyAura(
            boss, {NaxxSpellIds::LocustSwarm10, NaxxSpellIds::LocustSwarm10Alt, NaxxSpellIds::LocustSwarm25}) ||
        botAI->HasAura("locust swarm", boss))
    {
        if (dynamic_cast<FleeAction*>(action))
            return 0.0f;
    }
    return 1.0f;
}

float FourHorsemenGenericMultiplier::GetValue(Action* action)
{
    if (!action || !helper.UpdateBossAI())
        return 1.0f;

    // Opening grace: nothing aimed at a horseman lands until the tanks
    // have all four parked. Tanks are unaffected in practice — their taunt
    // is cast directly and their auto-attack is not an action — so they
    // still establish threat while the raid holds.
    if (helper.InPullGrace())
    {
        Unit* target = action->GetTarget();
        if (target && NaxxHelpers::IsHorseman(botAI, target))
            return 0.0f;

        return 1.0f;  // and no threat-neglect until the hold is over
    }

    // Upstream forced "neglect threat" here, telling every bot to ignore
    // threat entirely. On the one fight where Marks halve tank threat every
    // application — and where one of the four is held by a promoted damage
    // spec — that guarantees the damage rips bosses off their camps. Bots
    // now respect threat and throttle instead.
    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action)))
        return 0.0f;

    // Healers must not chase a heal target out of their quadrant. The camps
    // are 55-100y apart, so crossing means collecting a second horseman's
    // Mark, abandoning your own camp, and usually arriving too late anyway.
    // Each camp keeps its own healers; anything out of range is another
    // quadrant's problem.
    if (dynamic_cast<ReachPartyMemberToHealAction*>(action))
        return 0.0f;

    return 1.0f;
}

// float GothikGenericMultiplier::GetValue(Action* action)
// {
//     Unit* boss = AI_VALUE2(Unit*, "find target", "gothik the harvester");
//     if (!boss)
//     {
//         return 1.0f;
//     }
//     BossAI* boss_ai = dynamic_cast<BossAI*>(boss->GetAI());
//     EventMap* eventMap = boss_botAI->GetEvents();
//     uint32 curr_phase = eventMap->GetPhaseMask();
//     if (curr_phase == 1 && (dynamic_cast<FollowAction*>(action)))
//     {
//         return 0.0f;
//     }
//     if (curr_phase == 1 && (dynamic_cast<AttackAction*>(action)))
//     {
//         Unit* target = action->GetTarget();
//         if (target == boss)
//         {
//             return 0.0f;
//         }
//     }
//     return 1.0f;
// }

float GluthGenericMultiplier::GetValue(Action* action)
{
    if (!helper.UpdateBossAI())
        return 1.0f;

    if ((dynamic_cast<DpsAssistAction*>(action) || dynamic_cast<TankAssistAction*>(action) ||
         dynamic_cast<FleeAction*>(action) || dynamic_cast<CastDebuffSpellOnAttackerAction*>(action) ||
         dynamic_cast<CastStarfallAction*>(action)))
    {
        return 0.0f;
    }

    if (botAI->IsMainTank(bot))
    {
        Aura* aura = NaxxSpellIds::GetAnyAura(bot, {NaxxSpellIds::MortalWound10, NaxxSpellIds::MortalWound25});
        if (!aura)
        {
            // Fallback to name for custom spell data.
            aura = botAI->GetAura("mortal wound", bot, false, true);
        }
        if (aura && aura->GetStackAmount() >= 5)
        {
            if (dynamic_cast<CastTauntAction*>(action) || dynamic_cast<CastDarkCommandAction*>(action) ||
                dynamic_cast<CastHandOfReckoningAction*>(action) || dynamic_cast<CastGrowlAction*>(action))
            {
                return 0.0f;
            }
        }
    }
    if (dynamic_cast<PetAttackAction*>(action))
    {
        Unit* target = AI_VALUE(Unit*, "current target");
        if (helper.IsZombieChow(target))
            return 0.0f;
    }
    return 1.0f;
}
