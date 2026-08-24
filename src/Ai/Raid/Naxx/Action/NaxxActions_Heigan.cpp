#include "Playerbots.h"
#include "NaxxActions.h"
#include "InstanceScript.h"
#include "NaxxSpellIds.h"
#include "RaidDirector.h"
#include "Spell.h"
#include "Timer.h"

bool HeiganDanceAction::Execute(Event /*event*/)
{
    // Per-section standing spots, computed from instance_naxxramas.cpp's
    // own GetEruptionSection wedges and verified to map back to their own
    // index. The old upstream waypoints were EXACTLY REVERSED against the
    // core numbering (waypoint[0] sat in core section 3), which parked the
    // raid in the erupting mirror zone from wave one — the live symptom
    // was 'nobody dances and the tank is in the wrong spot'.
    static std::pair<float, float> const nearPoints[4] = {
        {2773.14f, -3702.70f},
        {2777.36f, -3694.22f},
        {2784.96f, -3686.98f},
        {2791.96f, -3684.53f},
    };
    static std::pair<float, float> const farPoints[4] = {
        {2752.26f, -3699.58f},
        {2760.04f, -3682.32f},
        {2774.70f, -3668.38f},
        {2788.33f, -3663.60f},
    };
    static std::pair<float, float> const platform = {2794.26f, -3706.67f};
    static float const platformZ = 276.54f;

    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
    if (!boss || !boss->IsAlive())
        return false;

    // Fast dance is marked by Plague Cloud (29350), which he casts on
    // himself for that phase. The previous check — "is Heigan near his
    // platform" — was measured against his own SPAWN point, so it read
    // true whenever the tank held him where he stands, which is the
    // correct phase-one position. Phase detection cannot depend on where
    // the raid chose to hold the boss.
    // The plan carries the script's own schedule: the fast dance's start
    // time is known 90 seconds in advance. Acting on the deadline instead
    // of the aura moves the departure from ~1.2s after the transition to
    // the transition itself — which is the difference between the far ring
    // making the first 7s eruption and eating it. The aura remains as the
    // fallback so an older IP build or a disabled director changes nothing.
    RaidPlan const* plan = RaidDirector::Get(bot);
    uint32 const now = getMSTime();
    bool const fastByPlan = plan && plan->nextEventKind == RAID_EVENT_HEIGAN_FAST_DANCE &&
                            plan->nextEventMs && now >= plan->nextEventMs;

    bool const fastPhase =
        fastByPlan || boss->HasAura(29350) || botAI->HasAura("plague cloud", boss);

    if (!fastPhase)
    {
        // Phase one: eruptions only fire on the floor below, so the whole
        // raid — melee included — fights from the platform. Sending melee
        // down to dance here left them unable to reach the boss at all.
        if (bot->GetDistance2d(platform.first, platform.second) < 12.0f)
            return false;

        return MoveInside(bot->GetMapId(), platform.first, platform.second, platformZ, 6.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    // DATA_HEIGAN_ERUPTION (naxxramas.h: 300) — reizan-core returns the
    // NEXT safe section here; 0 pre-fight matches the first wave.
    InstanceScript* instance = bot->GetInstanceScript();
    uint32 safe = instance ? instance->GetData(300) : 0;
    if (safe > 3)
        safe = 0;

    // Transition window: the script hard-resets to section 3 when the fast
    // dance begins, but the instance mirror still holds phase one's last
    // value until the first fast pulse fires. Anyone who trusted it would
    // dance to a stale wedge, so the entry window overrides it.
    if (fastByPlan && now < plan->nextEventMs + 7000)
        safe = 3;

    // Fast dance, everyone on the floor: melee take the near ring and
    // ranged the far one, so forty bots spread across the safe wedge
    // instead of stacking on one point.
    auto const& points = (PlayerbotAI::IsRanged(bot) || PlayerbotAI::IsHeal(bot)) ? farPoints : nearPoints;
    float const x = points[safe].first;
    float const y = points[safe].second;
    if (bot->GetDistance2d(x, y) < 6.0f)
        return false;

    if (fastPhase)
        botAI->InterruptSpell();

    return MoveInside(bot->GetMapId(), x, y, bot->GetPositionZ(), 4.0f, MovementPriority::MOVEMENT_COMBAT);
}

//bool HeiganDanceAction::CalculateSafe()
//{
//    Unit* boss = AI_VALUE2(Unit*, "find target", "heigan the unclean");
//    if (!boss)
//    {
//        return false;
//    }
//    uint32 now = getMSTime();
//    platform_phase = boss->IsWithinDist2d(platform.first, platform.second, 10.0f);
//    if (last_eruption_ms != 0 && now - last_eruption_ms > 15000)
//    {
//        ResetSafe();
//    }
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
//                if (last_eruption_ms == 0 || now - last_eruption_ms > 500)
//                {
//                    NextSafe();
//                }
//                last_eruption_ms = now;
//            }
//        }
//    }
//    return true;
//}
//
//bool HeiganDanceMeleeAction::Execute(Event event)
//{
//    CalculateSafe();
//    if (!platform_phase && botAI->IsMainTank(bot) && !AI_VALUE2(bool, "has aggro", "boss target"))
//    {
//        return false;
//    }
//    assert(curr_safe >= 0 && curr_safe <= 3);
//    return MoveInside(bot->GetMapId(), waypoints[curr_safe].first, waypoints[curr_safe].second, bot->GetPositionZ(),
//                      botAI->IsMainTank(bot) ? 0 : 0, MovementPriority::MOVEMENT_COMBAT);
//}
//
//bool HeiganDanceRangedAction::Execute(Event event)
//{
//    CalculateSafe();
//    if (!platform_phase)
//    {
//        if (MoveTo(bot->GetMapId(), platform.first, platform.second, 276.54f, false, false, false, false,
//                   MovementPriority::MOVEMENT_COMBAT))
//        {
//            return true;
//        }
//        return MoveInside(bot->GetMapId(), platform.first, platform.second, 276.54f, 2.0f,
//                          MovementPriority::MOVEMENT_COMBAT);
//    }
//    botAI->InterruptSpell();
//    return MoveInside(bot->GetMapId(), waypoints[curr_safe].first, waypoints[curr_safe].second, bot->GetPositionZ(), 0,
//                      MovementPriority::MOVEMENT_COMBAT);
//}
