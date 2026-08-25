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
    constexpr float cx = 3305.2f, cy = -3298.4f;

    // On the 45y ring, "the far side from the boss" is 57y+ away — out of
    // spell range. The clean floor near a kited boss is radially INWARD:
    // the trail is laid tangentially behind him along the ring, so the
    // inner ring on his own radial has been clear the longest while range
    // to him stays ~28y. Fanned by group slot around the inner ring so
    // Slime Spray never gets the whole camp in one cone; the camp orbits
    // with him as the kite advances.
    float dx = boss->GetPositionX() - cx;
    float dy = boss->GetPositionY() - cy;
    float const len = std::sqrt(dx * dx + dy * dy);
    if (len < 3.0f)
    {
        // Boss on the centre (pull, reset): fan toward the room's open side
        // instead of dividing by zero.
        dx = 1.0f;
        dy = 0.0f;
    }
    else
    {
        dx /= len;
        dy /= len;
    }

    float const baseAngle = std::atan2(dy, dx);
    int32 slot = botAI->GetGroupSlotIndex(bot);
    if (slot < 0)
        slot = 0;

    float const angle = baseAngle + (float(slot % 7) - 3.0f) * 0.22f;
    float x = cx + std::cos(angle) * 17.0f;
    float y = cy + std::sin(angle) * 17.0f;
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
