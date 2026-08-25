#include "NaxxActions.h"

#include "Playerbots.h"

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

bool GrobbulusRangedPositionAction::Execute(Event /*event*/)
{
    Unit* boss = AI_VALUE(Unit*, "boss target");
    if (!boss)
        boss = AI_VALUE2(Unit*, "find target", "grobbulus");
    if (!boss)
        return false;

    // Kite ring centre (same constant as the rotation).
    constexpr float cx = 3281.23f, cy = -3310.38f;

    float dx = cx - boss->GetPositionX();
    float dy = cy - boss->GetPositionY();
    float const len = std::sqrt(dx * dx + dy * dy);
    if (len < 3.0f)
    {
        // Boss on the centre (pull, reset): fan out toward the room's open
        // side instead of dividing by zero.
        dx = 1.0f;
        dy = 0.0f;
    }
    else
    {
        dx /= len;
        dy /= len;
    }

    // Continue through the centre to the far side, fanned by group slot so
    // Slime Spray never gets the whole camp in one cone.
    float const baseAngle = std::atan2(dy, dx);
    int32 slot = botAI->GetGroupSlotIndex(bot);
    if (slot < 0)
        slot = 0;

    float const angle = baseAngle + (float(slot % 7) - 3.0f) * 0.35f;
    float x = cx + std::cos(angle) * 12.0f;
    float y = cy + std::sin(angle) * 12.0f;
    float z = bot->GetPositionZ();
    bot->UpdateAllowedPositionZ(x, y, z);

    if (bot->GetExactDist2d(x, y) < 6.0f)
        return false;

    return MoveInside(bot->GetMapId(), x, y, z, 3.0f, MovementPriority::MOVEMENT_COMBAT);
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

uint32 GrobbulusRotateAction::GetCurrWaypoint()
{
    uint32 current = FindNearestWaypoint();
    if (clockwise)
        return (current + 1) % intervals;

    return (current + intervals - 1) % intervals;
}
