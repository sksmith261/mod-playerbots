#include "NaxxActions.h"

#include <algorithm>

#include "RaidDirector.h"
#include "NaxxRaidPlans.h"
#include "Playerbots.h"

uint32 RotateAroundTheCenterPointAction::FindNearestWaypoint()
{
    float minDistance = 0;
    int ret = -1;
    for (uint32 i = 0; i < intervals; i++)
    {
        float w_x = waypoints[i].first, w_y = waypoints[i].second;
        float dis = bot->GetDistance2d(w_x, w_y);
        if (ret == -1 || dis < minDistance)
        {
            ret = i;
            minDistance = dis;
        }
    }
    return ret;
}

// Baseline execution for any Naxx boss without bespoke handling. Runs BELOW
// every specialist action, so dances, kiting and side splits win whenever
// they have something to say; this only fills the silence.
bool NaxxPlanAction::Execute(Event /*event*/)
{
    RaidPlan const* plan = RaidDirector::Get(bot);
    if (!plan || plan->encounter != RAID_ENCOUNTER_NAXX_GENERIC)
        return false;

    RaidAssignment const* mine = plan->For(bot->GetGUID());
    if (!mine)
        return false;

    Unit* boss = mine->target ? ObjectAccessor::GetUnit(*bot, mine->target) : nullptr;
    if (!boss || !boss->IsAlive())
        return false;

    // camp 0 means this encounter's own logic owns positioning.
    if (mine->camp > 0)
    {
        float const ring = NaxxRaidPlans::GenericRing(plan->label);
        if (ring > 0.0f)
        {
            // Spread over however many slots were actually dealt. The old
            // fixed 0.45 rad step wrapped past a full circle at slot
            // fourteen, so larger raids stacked two bots on every slot from
            // there on.
            float const angle =
                2.0f * float(M_PI) * float(mine->camp) / float(std::max(plan->ringSlots, 1u));
            float x = boss->GetPositionX() + ring * std::cos(angle);
            float y = boss->GetPositionY() + ring * std::sin(angle);
            float z = bot->GetPositionZ();
            bot->UpdateAllowedPositionZ(x, y, z);

            if (bot->GetExactDist2d(x, y) > 6.0f)
                return MoveInside(bot->GetMapId(), x, y, z, 4.0f, MovementPriority::MOVEMENT_COMBAT);
        }
    }

    if (mine->duty == RAID_DUTY_HEAL)
        return false;  // positioned; the heal engine owns the rest

    // Targeting is opt-in per encounter. Every bespoke choose-target action
    // ends with "already on the right target -> return false", and that
    // false hands this very tick to us. Issuing Attack(boss) here yanked
    // bots off their crypt guard, wave add, web wrap or spore every other
    // tick, and the bespoke action yanked them straight back. The plan only
    // targets where nothing bespoke does.
    if (!NaxxRaidPlans::GenericPlanTargets(plan->label))
        return false;

    if (AI_VALUE(Unit*, "current target") == boss)
        return false;

    return Attack(boss);
}
