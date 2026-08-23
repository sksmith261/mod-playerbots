#include "NaxxActions.h"

#include "Playerbots.h"

#include <cmath>

bool GrobbulusGoBehindAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE(Unit*, "boss target");
    if (!boss)
        return false;

    // Position* pos = boss->GetPosition();
    float orientation = boss->GetOrientation() + M_PI + delta_angle;
    float x = boss->GetPositionX();
    float y = boss->GetPositionY();
    float z = boss->GetPositionZ();
    float rx = x + cos(orientation) * distance;
    float ry = y + sin(orientation) * distance;
    return MoveTo(bot->GetMapId(), rx, ry, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool GrobbulusMoveAwayAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE(Unit*, "boss target");
    if (!boss)
        return false;

    const float currentDistance = bot->GetExactDist2d(boss);
    if (currentDistance >= distance)
        return false;

    const float angle = boss->GetAngle(bot);
    const float x = boss->GetPositionX() + cos(angle) * distance;
    const float y = boss->GetPositionY() + sin(angle) * distance;
    const float z = bot->GetPositionZ();

    return MoveTo(bot->GetMapId(), x, y, z, false, false, false, false, MovementPriority::MOVEMENT_COMBAT);
}

bool GrobbulusRotateAction::Execute(Event event)
{
    // The ring was hardcoded around (3281.23, -3310.38) — a point with
    // nothing within 40y of it, and 82y from where Grobbulus actually
    // spawns. Reaching it meant kiting him out of the room entirely, walls
    // included. Anchor on his own spawn instead and rebuild the waypoints
    // around that.
    Unit* boss = AI_VALUE(Unit*, "boss target");
    Creature* creature = boss ? boss->ToCreature() : nullptr;
    if (creature)
    {
        Position const& home = creature->GetHomePosition();
        if (std::fabs(home.GetPositionX() - center_x) > 1.0f ||
            std::fabs(home.GetPositionY() - center_y) > 1.0f)
        {
            center_x = home.GetPositionX();
            center_y = home.GetPositionY();

            waypoints.clear();
            for (uint32 i = 0; i < intervals; ++i)
            {
                float const angle = 2.0f * float(M_PI) * i / intervals;
                waypoints.push_back(std::make_pair(center_x + std::cos(angle) * radius,
                                                   center_y + std::sin(angle) * radius));
            }
        }
    }

    return RotateAroundTheCenterPointAction::Execute(event);
}

uint32 GrobbulusRotateAction::GetCurrWaypoint()
{
    uint32 current = FindNearestWaypoint();
    if (clockwise)
        return (current + 1) % intervals;

    return (current + intervals - 1) % intervals;
}
