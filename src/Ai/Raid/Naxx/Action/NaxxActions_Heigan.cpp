#include "Playerbots.h"
#include "NaxxActions.h"
#include "InstanceScript.h"
#include "NaxxSpellIds.h"
#include "Spell.h"
#include "Timer.h"

bool HeiganDanceAction::Execute(Event /*event*/)
{
    // Floor standing spots, one per eruption section 0..3 (from the old
    // implementation — geometry unchanged), plus the ranged platform.
    static std::pair<float, float> const sectionPoints[4] = {
        {2794.88f, -3668.12f},
        {2775.49f, -3674.43f},
        {2762.30f, -3684.59f},
        {2755.99f, -3703.96f},
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

    float const x = sectionPoints[safe].first;
    float const y = sectionPoints[safe].second;
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
