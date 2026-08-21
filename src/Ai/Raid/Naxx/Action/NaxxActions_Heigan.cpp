#include "Playerbots.h"
#include "NaxxActions.h"
#include "InstanceScript.h"
#include "NaxxSpellIds.h"
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

    // Fast dance: Heigan teleports onto his platform and clouds it —
    // everyone must be on the floor dancing.
    bool const fastPhase = boss->IsWithinDist2d(platform.first, platform.second, 12.0f);

    if (!fastPhase && botAI->IsRanged(bot))
    {
        // Slow phase: casters and healers camp the platform; there are no
        // eruption gameobjects up there, so it is safe the entire phase.
        if (bot->GetDistance2d(platform.first, platform.second) < 5.0f)
            return false;

        return MoveInside(bot->GetMapId(), platform.first, platform.second, platformZ, 2.0f,
                          MovementPriority::MOVEMENT_COMBAT);
    }

    // DATA_HEIGAN_ERUPTION (naxxramas.h: 300) — reizan-core returns the
    // NEXT safe section here; 0 pre-fight matches the first wave.
    InstanceScript* instance = bot->GetInstanceScript();
    uint32 safe = instance ? instance->GetData(300) : 0;
    if (safe > 3)
        safe = 0;

    // Slow phase: tank and melee dance the near ring so Heigan barely
    // moves and stays in reach. Fast dance: everyone uses the far ring,
    // deeper inside each wedge for margin at 4s wave cadence.
    auto const& points = fastPhase ? farPoints : nearPoints;
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
